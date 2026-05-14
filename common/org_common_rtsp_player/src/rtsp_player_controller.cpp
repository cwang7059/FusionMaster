#include "rtsp_player_controller.h"

#include <plugin_api/iplugin_manager.h>
#include <plugin_api/ireplay_runtime_state.h>
#include <plugin_api/rtsp_packet_format.h>

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QMutexLocker>
#include <QTimer>
#include <QUuid>
#include <QVariantMap>

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <mutex>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

namespace {

constexpr int kUiRefreshIntervalMs = 500;
constexpr int kFpsWindowMs = 1000;
constexpr int kTargetLatencyMs = 120;
constexpr int kDefaultPresentationIntervalMs = 16;
constexpr int kMinPresentationIntervalMs = 16;
constexpr int kMaxPresentationIntervalMs = 16;
constexpr int kSelectedLowLatencyMaxQueuedPackets = 5;
constexpr qint64 kSelectedLowLatencyMaxQueuedBytes = 2LL * 1024LL * 1024LL;
constexpr int kSelectedPreferIntegrityMaxQueuedPackets = 720;
constexpr qint64 kSelectedPreferIntegrityMaxQueuedBytes = 64LL * 1024LL * 1024LL;
constexpr int kBackgroundMaxQueuedPackets = 3;
constexpr qint64 kBackgroundMaxQueuedBytes = 1LL * 1024LL * 1024LL;
constexpr auto kRtspReplayControlTopic = "rtsp/replay/control";
constexpr auto kRtspReplayControlResultTopic = "rtsp/replay/control/result";

qint64 nowMs() {
    return QDateTime::currentMSecsSinceEpoch();
}

QByteArray copyExtradata(const QByteArray& value) {
    if (value.isEmpty()) {
        return QByteArray();
    }
    return QByteArray(value.constData(), value.size());
}

bool isRtspUrl(const QString& value) {
    const QString lower = value.trimmed().toLower();
    return lower.startsWith(QStringLiteral("rtsp://"))
        || lower.startsWith(QStringLiteral("rtsps://"));
}

QString sanitizeStreamId(const QString& value, int index) {
    QString out;
    out.reserve(value.size());

    for (const QChar ch : value.trimmed()) {
        if (ch.isLetterOrNumber() || ch == QChar('_') || ch == QChar('-')) {
            out.push_back(ch.toLower());
        } else {
            out.push_back(QChar('_'));
        }
    }

    if (out.isEmpty()) {
        out = QStringLiteral("stream%1").arg(index + 1);
    }

    return out;
}

QString makeUniqueStreamId(const QString& requestedId, const QSet<QString>& usedIds, int indexHint) {
    const QString base = sanitizeStreamId(requestedId, indexHint);
    QString candidate = base;
    int suffix = 2;
    while (usedIds.contains(candidate)) {
        candidate = QStringLiteral("%1_%2").arg(base).arg(suffix);
        ++suffix;
    }
    return candidate;
}

QStringList splitBySemicolon(const QString& raw) {
    const QStringList parts = raw.split(QStringLiteral(";"));
    QStringList cleaned;
    for (const QString& part : parts) {
        const QString token = part.trimmed();
        if (!token.isEmpty()) {
            cleaned.push_back(token);
        }
    }
    return cleaned;
}

bool preferPacketIntegrityByDefault() {
    const QString value = qEnvironmentVariable("OSGI_RTSP_PLAYER_PACKET_MODE").trimmed().toLower();
    if (value == QStringLiteral("low_latency")
        || value == QStringLiteral("lowlatency")
        || value == QStringLiteral("realtime")) {
        return false;
    }
    if (value == QStringLiteral("prefer_integrity")
        || value == QStringLiteral("less_drop")
        || value == QStringLiteral("integrity")
        || value == QStringLiteral("reliable")) {
        return true;
    }
    return false;
}

}  // namespace

struct RtspPlayerController::EncodedPacket {
    plugin_api::RtspPacketPayload payload;
};

struct RtspPlayerController::StreamState {
    QString streamId;
    QString topic;

    std::atomic<bool> stop{false};
    std::atomic<bool> resetPending{false};
    std::atomic<bool> waitingForKeyFrame{true};
    std::thread worker;

    std::mutex queueMutex;
    std::condition_variable queueCv;
    std::deque<EncodedPacket> queue;
    qint64 queuedBytes = 0;
    std::atomic<int> queuedPacketCount{0};

    std::atomic<qint64> receivedPacketCount{0};
    std::atomic<qint64> decodedFrameCount{0};
    std::atomic<qint64> droppedPacketCount{0};
    std::atomic<qint64> lastWallClockUs{0};
    std::atomic<int> codecId{0};
    std::atomic<int> frameWidth{0};
    std::atomic<int> frameHeight{0};
    std::atomic<int> fps{0};

    qint64 fpsWindowStartMs = 0;
    int fpsWindowFrames = 0;

    QByteArray decoderExtradata;
    AVCodecContext* codecContext = nullptr;
    AVFrame* decodedFrame = nullptr;
    AVPacket* packet = nullptr;
    SwsContext* swsContext = nullptr;
    int swsSrcW = 0;
    int swsSrcH = 0;
    AVPixelFormat swsSrcFormat = AV_PIX_FMT_NONE;
    int swsFlags = 0;
    bool decodeActive = false;

    QMutex lastFrameMutex;
    QImage lastFrame;
};

RtspPlayerController::RtspPlayerController(QObject* parent)
    : QObject(parent) {
    preferPacketIntegrity_.store(preferPacketIntegrityByDefault(), std::memory_order_release);
    framePresentationIntervalMs_ = kDefaultPresentationIntervalMs;

    uiRefreshTimer_ = new QTimer(this);
    uiRefreshTimer_->setInterval(kUiRefreshIntervalMs);
    connect(uiRefreshTimer_, &QTimer::timeout, this, [this]() {
        refreshViewState();
    });

    framePresentTimer_ = new QTimer(this);
    framePresentTimer_->setTimerType(Qt::PreciseTimer);
    framePresentTimer_->setInterval(framePresentationIntervalMs_);
    connect(framePresentTimer_, &QTimer::timeout, this, [this]() {
        deliverPendingFrame();
    });
}

RtspPlayerController::~RtspPlayerController() {
    stop();
}

void RtspPlayerController::setPluginManager(IPluginManager* pluginManager) {
    pluginManager_ = pluginManager;
}

void RtspPlayerController::start() {
    if (pluginManager_ == nullptr) {
        qWarning() << "[RTSP_PLAYER] PluginManager 不可用";
        return;
    }

    if (messageBus_ != nullptr) {
        return;
    }

    messageBus_ = pluginManager_->messageBus();
    replayActive_.store(false, std::memory_order_release);
    initializeConfiguredPullUrls();

    if (auto* replayState = pluginManager_->getService<IReplayRuntimeState>(QStringLiteral("replay_runtime_state"))) {
        replayActive_.store(replayState->replayActive(), std::memory_order_release);
    }

    if (messageBus_ != nullptr) {
        rtspSubscriptionId_ = messageBus_->subscribeWithTopicRaw(
            QStringLiteral("rtsp/raw/"),
            [this](const QString& topic, const QByteArray& payload) {
                handleRtspPacket(topic, payload);
            });

        replayStatusSubscriptionId_ = messageBus_->subscribeWithTopic(
            QStringLiteral("record_replay/status"),
            [this](const QString&, const QByteArray& payload) {
                bool replayActive = false;
                if (!parseReplayActiveEvent(payload, &replayActive)) {
                    return;
                }
                handleReplayStateChanged(replayActive);
            });

        controlResultSubscriptionId_ = messageBus_->subscribeWithTopic(
            QString::fromLatin1(kRtspReplayControlResultTopic),
            [this](const QString&, const QByteArray& payload) {
                QMetaObject::invokeMethod(
                    this,
                    [this, payload]() {
                        handleControlResult(payload);
                    },
                    Qt::QueuedConnection);
            });
    }

    uiRefreshTimer_->start();
    refreshViewState();
}

void RtspPlayerController::stop() {
    if (messageBus_ != nullptr) {
        if (rtspSubscriptionId_ != 0) {
            messageBus_->unsubscribe(rtspSubscriptionId_);
        }
        if (replayStatusSubscriptionId_ != 0) {
            messageBus_->unsubscribe(replayStatusSubscriptionId_);
        }
        if (controlResultSubscriptionId_ != 0) {
            messageBus_->unsubscribe(controlResultSubscriptionId_);
        }
    }

    rtspSubscriptionId_ = 0;
    replayStatusSubscriptionId_ = 0;
    controlResultSubscriptionId_ = 0;

    if (uiRefreshTimer_ != nullptr) {
        uiRefreshTimer_->stop();
    }
    if (framePresentTimer_ != nullptr) {
        framePresentTimer_->stop();
    }

    stopAllStreams();

    {
        QMutexLocker locker(&frameMutex_);
        pendingFrame_ = QImage();
        pendingFrameStreamId_.clear();
        frameDispatchPending_ = false;
    }

    selectedStreamId_.clear();
    preferredStreamId_.clear();
    addStreamStatusText_.clear();
    {
        QMutexLocker locker(&streamsMutex_);
        streamPullUrls_.clear();
        pendingRequestUrls_.clear();
    }
    replayActive_.store(false, std::memory_order_release);
    messageBus_ = nullptr;

    emit frameCleared();
    emit streamsChanged();
    emit selectedStreamIdChanged();
    emit playbackModeChanged();
    emit statusTextChanged();
    emit addStreamStatusTextChanged();
    emit selectedStreamInfoChanged();
}

QVariantList RtspPlayerController::streams() const {
    QList<std::shared_ptr<StreamState>> orderedStreams;
    {
        QMutexLocker locker(&streamsMutex_);
        orderedStreams = streams_.values();
    }

    std::sort(orderedStreams.begin(), orderedStreams.end(), [](const auto& lhs, const auto& rhs) {
        return lhs->streamId < rhs->streamId;
    });

    QVariantList rows;
    rows.reserve(orderedStreams.size());

    for (const auto& stream : orderedStreams) {
        if (stream == nullptr) {
            continue;
        }

        QVariantMap row;
        row.insert(QStringLiteral("id"), stream->streamId);
        row.insert(QStringLiteral("title"), stream->streamId);
        row.insert(QStringLiteral("url"), pullUrlForStream(stream->streamId));
        row.insert(QStringLiteral("topic"), stream->topic);
        row.insert(QStringLiteral("codec"), codecName(stream->codecId.load(std::memory_order_acquire)));
        row.insert(QStringLiteral("packets"), stream->receivedPacketCount.load(std::memory_order_acquire));
        row.insert(QStringLiteral("frames"), stream->decodedFrameCount.load(std::memory_order_acquire));
        row.insert(QStringLiteral("fps"), stream->fps.load(std::memory_order_acquire));
        row.insert(
            QStringLiteral("resolution"),
            QStringLiteral("%1 x %2")
                .arg(stream->frameWidth.load(std::memory_order_acquire))
                .arg(stream->frameHeight.load(std::memory_order_acquire)));
        row.insert(
            QStringLiteral("waitingForKeyFrame"),
            stream->waitingForKeyFrame.load(std::memory_order_acquire));
        rows.push_back(row);
    }

    return rows;
}

QString RtspPlayerController::selectedStreamId() const {
    QMutexLocker locker(&streamsMutex_);
    return selectedStreamId_;
}

void RtspPlayerController::setSelectedStreamId(const QString& streamId) {
    const QString normalizedId = streamId.trimmed();
    {
        QMutexLocker locker(&streamsMutex_);
        if (selectedStreamId_ == normalizedId) {
            return;
        }
        selectedStreamId_ = normalizedId;
        preferredStreamId_.clear();
    }

    emit selectedStreamIdChanged();
    emit frameCleared();
    emit selectedStreamInfoChanged();
    emit statusTextChanged();

    QList<std::shared_ptr<StreamState>> allStreams;
    {
        QMutexLocker locker(&streamsMutex_);
        allStreams = streams_.values();
    }
    for (const auto& stream : allStreams) {
        if (stream == nullptr) {
            continue;
        }
        trimQueueToBudget(stream.get());
        stream->queueCv.notify_all();
    }

    const auto currentStream = streamState(normalizedId);
    if (currentStream != nullptr) {
        QImage lastFrame;
        {
            QMutexLocker frameLocker(&currentStream->lastFrameMutex);
            lastFrame = currentStream->lastFrame;
        }
        if (!lastFrame.isNull()) {
            queueFramePresentation(normalizedId, lastFrame);
        }
    }
}

bool RtspPlayerController::replayActive() const {
    return replayActive_.load(std::memory_order_acquire);
}

bool RtspPlayerController::preferPacketIntegrity() const {
    return preferPacketIntegrity_.load(std::memory_order_acquire);
}

void RtspPlayerController::setPreferPacketIntegrity(bool value) {
    const bool previous = preferPacketIntegrity_.exchange(value, std::memory_order_acq_rel);
    if (previous == value) {
        return;
    }

    QList<std::shared_ptr<StreamState>> allStreams;
    {
        QMutexLocker locker(&streamsMutex_);
        allStreams = streams_.values();
    }

    for (const auto& stream : allStreams) {
        if (stream == nullptr) {
            continue;
        }
        trimQueueToBudget(stream.get());
        stream->queueCv.notify_all();
    }

    emit packetHandlingModeChanged();
    emit statusTextChanged();
    emit selectedStreamInfoChanged();
}

QString RtspPlayerController::packetHandlingModeText() const {
    return preferPacketIntegrity()
        ? QStringLiteral("prefer_integrity")
        : QStringLiteral("target_latency_120ms");
}

QString RtspPlayerController::playbackModeText() const {
    return replayActive() ? QStringLiteral("消费重演流") : QStringLiteral("实时流");
}

QString RtspPlayerController::statusText() const {
    const auto currentStream = streamState(selectedStreamId());
    if (currentStream == nullptr) {
        return QStringLiteral("%1，等待 RTSP 编码包").arg(playbackModeText());
    }

    if (currentStream->waitingForKeyFrame.load(std::memory_order_acquire)) {
        return QStringLiteral("%1，等待关键帧").arg(playbackModeText());
    }

    if (currentStream->decodedFrameCount.load(std::memory_order_acquire) <= 0) {
        return QStringLiteral("%1，等待可解码画面").arg(playbackModeText());
    }

    return QStringLiteral("%1，正在渲染 %2").arg(playbackModeText(), currentStream->streamId);
}

QString RtspPlayerController::addStreamStatusText() const {
    return addStreamStatusText_;
}

QString RtspPlayerController::selectedTopic() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr ? QString() : currentStream->topic;
}

QString RtspPlayerController::selectedCodecText() const {
    const auto currentStream = streamState(selectedStreamId());
    if (currentStream == nullptr) {
        return QStringLiteral("unknown");
    }
    return codecName(currentStream->codecId.load(std::memory_order_acquire));
}

QString RtspPlayerController::selectedResolutionText() const {
    const auto currentStream = streamState(selectedStreamId());
    if (currentStream == nullptr) {
        return QStringLiteral("--");
    }

    const int width = currentStream->frameWidth.load(std::memory_order_acquire);
    const int height = currentStream->frameHeight.load(std::memory_order_acquire);
    if (width <= 0 || height <= 0) {
        return QStringLiteral("--");
    }

    return QStringLiteral("%1 x %2").arg(width).arg(height);
}

qint64 RtspPlayerController::receivedPacketCount() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr ? 0 : currentStream->receivedPacketCount.load(std::memory_order_acquire);
}

qint64 RtspPlayerController::decodedFrameCount() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr ? 0 : currentStream->decodedFrameCount.load(std::memory_order_acquire);
}

int RtspPlayerController::decodedFps() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr ? 0 : currentStream->fps.load(std::memory_order_acquire);
}

bool RtspPlayerController::waitingForKeyFrame() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr ? true : currentStream->waitingForKeyFrame.load(std::memory_order_acquire);
}

QString RtspPlayerController::lastFrameTimeText() const {
    const auto currentStream = streamState(selectedStreamId());
    if (currentStream == nullptr) {
        return QStringLiteral("--");
    }

    return formatWallClock(currentStream->lastWallClockUs.load(std::memory_order_acquire));
}

int RtspPlayerController::selectedQueuedPacketCount() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr
        ? 0
        : currentStream->queuedPacketCount.load(std::memory_order_acquire);
}

qint64 RtspPlayerController::selectedDroppedPacketCount() const {
    const auto currentStream = streamState(selectedStreamId());
    return currentStream == nullptr
        ? 0
        : currentStream->droppedPacketCount.load(std::memory_order_acquire);
}

void RtspPlayerController::togglePacketHandlingMode() {
    setPreferPacketIntegrity(!preferPacketIntegrity());
}

void RtspPlayerController::requestAddStream(const QString& url, const QString& streamId) {
    const QString trimmedUrl = url.trimmed();
    const QString trimmedStreamId = streamId.trimmed();

    if (messageBus_ == nullptr) {
        addStreamStatusText_ = QStringLiteral("消息总线不可用，暂时无法添加 RTSP 流");
        emit addStreamStatusTextChanged();
        return;
    }

    if (!isRtspUrl(trimmedUrl)) {
        addStreamStatusText_ = QStringLiteral("请输入有效的 RTSP 地址，例如 rtsp://127.0.0.1:8554/live/cam1");
        emit addStreamStatusTextChanged();
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("action"), QStringLiteral("add_stream"));
    root.insert(
        QStringLiteral("requestId"),
        QUuid::createUuid().toString(QUuid::WithoutBraces));
    root.insert(QStringLiteral("streamId"), trimmedStreamId);
    root.insert(QStringLiteral("url"), trimmedUrl);

    addStreamStatusText_ = replayActive()
        ? QStringLiteral("已提交添加请求，当前处于重演模式，退出重演后会自动启动")
        : QStringLiteral("已提交添加请求，正在启动 RTSP 拉流");
    emit addStreamStatusTextChanged();

    publishControlRequest(root);
}

void RtspPlayerController::requestRemoveSelectedStream() {
    const QString streamId = selectedStreamId().trimmed();
    if (messageBus_ == nullptr) {
        addStreamStatusText_ = QStringLiteral("消息总线不可用，暂时无法删除实时流");
        emit addStreamStatusTextChanged();
        return;
    }

    if (streamId.isEmpty()) {
        addStreamStatusText_ = QStringLiteral("当前没有可删除的实时流");
        emit addStreamStatusTextChanged();
        return;
    }

    if (replayActive()) {
        addStreamStatusText_ = QStringLiteral("重演模式下不支持删除实时流");
        emit addStreamStatusTextChanged();
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("action"), QStringLiteral("remove_stream"));
    root.insert(
        QStringLiteral("requestId"),
        QUuid::createUuid().toString(QUuid::WithoutBraces));
    root.insert(QStringLiteral("streamId"), streamId);

    addStreamStatusText_ = QStringLiteral("已提交删除请求，正在移除流 %1").arg(streamId);
    emit addStreamStatusTextChanged();

    publishControlRequest(root);
}

void RtspPlayerController::requestRemoveStream(const QString& streamId) {
    const QString previousSelection = selectedStreamId();
    if (previousSelection != streamId) {
        setSelectedStreamId(streamId);
    }

    requestRemoveSelectedStream();

    if (previousSelection != streamId && previousSelection != selectedStreamId()) {
        setSelectedStreamId(previousSelection);
    }
}

void RtspPlayerController::handleRtspPacket(const QString& topic, const QByteArray& payload) {
    plugin_api::RtspPacketPayload packet;
    if (!plugin_api::parseRtspPacketPayload(payload, &packet)) {
        return;
    }

    const QString streamId = plugin_api::rtspStreamIdFromTopic(topic);
    const auto stream = ensureStreamState(streamId, topic);
    if (stream == nullptr) {
        return;
    }

    stream->receivedPacketCount.fetch_add(1, std::memory_order_acq_rel);
    stream->lastWallClockUs.store(packet.wallClockUs, std::memory_order_release);
    stream->codecId.store(packet.codecId, std::memory_order_release);

    EncodedPacket queuedPacket;
    queuedPacket.payload = std::move(packet);
    const QueueBudget budget = queueBudgetForStream(streamId);

    {
        std::lock_guard<std::mutex> lock(stream->queueMutex);
        const qint64 packetBytes =
            static_cast<qint64>(queuedPacket.payload.encoded.size()) + static_cast<qint64>(queuedPacket.payload.extradata.size());

        while (!stream->queue.empty()
               && (stream->queuedPacketCount.load(std::memory_order_acquire) >= budget.maxPackets
                   || stream->queuedBytes + packetBytes > budget.maxBytes)) {
            stream->queuedBytes -= static_cast<qint64>(stream->queue.front().payload.encoded.size())
                + static_cast<qint64>(stream->queue.front().payload.extradata.size());
            stream->queue.pop_front();
            stream->queuedPacketCount.fetch_sub(1, std::memory_order_acq_rel);
            stream->droppedPacketCount.fetch_add(1, std::memory_order_acq_rel);
        }

        stream->queuedBytes += packetBytes;
        stream->queue.push_back(std::move(queuedPacket));
        stream->queuedPacketCount.fetch_add(1, std::memory_order_acq_rel);
    }

    stream->queueCv.notify_one();
}

RtspPlayerController::QueueBudget RtspPlayerController::queueBudgetForStream(const QString& streamId) const {
    const QString normalizedId = streamId.trimmed();

    QMutexLocker locker(&streamsMutex_);
    const QString selectedId = selectedStreamId_.trimmed();
    const bool decodeSelectedOnly = !selectedId.isEmpty();
    const bool isSelected = !decodeSelectedOnly || normalizedId == selectedId;

    if (!isSelected) {
        return {kBackgroundMaxQueuedPackets, kBackgroundMaxQueuedBytes};
    }

    if (preferPacketIntegrity_.load(std::memory_order_acquire)) {
        return {kSelectedPreferIntegrityMaxQueuedPackets, kSelectedPreferIntegrityMaxQueuedBytes};
    }

    return {kSelectedLowLatencyMaxQueuedPackets, kSelectedLowLatencyMaxQueuedBytes};
}

bool RtspPlayerController::shouldDecodeStream(const QString& streamId) const {
    const QString normalizedId = streamId.trimmed();
    QMutexLocker locker(&streamsMutex_);
    const QString selectedId = selectedStreamId_.trimmed();
    return selectedId.isEmpty() || normalizedId == selectedId;
}

void RtspPlayerController::trimQueueToBudget(StreamState* stream) {
    if (stream == nullptr) {
        return;
    }

    const QueueBudget budget = queueBudgetForStream(stream->streamId);
    std::lock_guard<std::mutex> lock(stream->queueMutex);
    while (!stream->queue.empty()
           && (stream->queuedPacketCount.load(std::memory_order_acquire) > budget.maxPackets
               || stream->queuedBytes > budget.maxBytes)) {
        stream->queuedBytes -= static_cast<qint64>(stream->queue.front().payload.encoded.size())
            + static_cast<qint64>(stream->queue.front().payload.extradata.size());
        stream->queue.pop_front();
        stream->queuedPacketCount.fetch_sub(1, std::memory_order_acq_rel);
        stream->droppedPacketCount.fetch_add(1, std::memory_order_acq_rel);
    }
}

int RtspPlayerController::swsFlagsForCurrentMode() const {
    return preferPacketIntegrity()
        ? (SWS_BILINEAR | SWS_ACCURATE_RND)
        : SWS_FAST_BILINEAR;
}

void RtspPlayerController::handleReplayStateChanged(bool replayActiveValue) {
    const bool previous = replayActive_.exchange(replayActiveValue, std::memory_order_acq_rel);
    if (previous == replayActiveValue) {
        return;
    }

    resetAllStreamStates();
    emit playbackModeChanged();
}

void RtspPlayerController::publishControlRequest(const QJsonObject& request) {
    if (messageBus_ == nullptr) {
        return;
    }

    const QString action = request.value(QStringLiteral("action")).toString().trimmed();
    const QString requestId = request.value(QStringLiteral("requestId")).toString().trimmed();
    const QString url = request.value(QStringLiteral("url")).toString().trimmed();
    if (action == QStringLiteral("add_stream") && !requestId.isEmpty() && !url.isEmpty()) {
        QMutexLocker locker(&streamsMutex_);
        pendingRequestUrls_.insert(requestId, url);
    }

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact);
    QMetaObject::invokeMethod(
        this,
        [this, payload]() {
            if (messageBus_ == nullptr) {
                return;
            }

            messageBus_->publish(QString::fromLatin1(kRtspReplayControlTopic), payload);
        },
        Qt::QueuedConnection);
}

void RtspPlayerController::initializeConfiguredPullUrls() {
    const QString raw = qEnvironmentVariable("OSGI_RTSP_URLS").trimmed();
    QHash<QString, QString> configuredPullUrls;
    if (!raw.isEmpty()) {
        const QStringList tokens = splitBySemicolon(raw);
        QSet<QString> usedIds;
        int index = 0;

        for (const QString& token : tokens) {
            QString streamId;
            QString url;

            if (isRtspUrl(token)) {
                url = token;
            } else {
                const int splitPos = token.indexOf('=');
                if (splitPos > 0) {
                    streamId = token.left(splitPos).trimmed();
                    url = token.mid(splitPos + 1).trimmed();
                } else {
                    url = token;
                }
            }

            if (!isRtspUrl(url)) {
                continue;
            }

            const QString normalizedId = makeUniqueStreamId(streamId, usedIds, index);
            usedIds.insert(normalizedId);
            configuredPullUrls.insert(normalizedId, url);
            ++index;
        }
    }

    QMutexLocker locker(&streamsMutex_);
    streamPullUrls_ = std::move(configuredPullUrls);
    pendingRequestUrls_.clear();
}

void RtspPlayerController::resetAllStreamStates() {
    QList<std::shared_ptr<StreamState>> allStreams;
    {
        QMutexLocker locker(&streamsMutex_);
        allStreams = streams_.values();
    }

    for (const auto& stream : allStreams) {
        if (stream == nullptr) {
            continue;
        }

        stream->waitingForKeyFrame.store(true, std::memory_order_release);
        stream->resetPending.store(true, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lock(stream->queueMutex);
            stream->queue.clear();
            stream->queuedBytes = 0;
            stream->queuedPacketCount.store(0, std::memory_order_release);
        }
        stream->queueCv.notify_all();

        QMutexLocker frameLocker(&stream->lastFrameMutex);
        stream->lastFrame = QImage();
    }

    {
        QMutexLocker locker(&frameMutex_);
        pendingFrame_ = QImage();
        pendingFrameStreamId_.clear();
        frameDispatchPending_ = false;
    }

    emit streamsChanged();
    emit statusTextChanged();
    emit selectedStreamInfoChanged();
    emit frameCleared();
}

void RtspPlayerController::handleControlResult(const QByteArray& payload) {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return;
    }

    const QJsonObject root = document.object();
    const QString action = root.value(QStringLiteral("action")).toString().trimmed();
    const QString message = root.value(QStringLiteral("message")).toString().trimmed();
    const QString requestId = root.value(QStringLiteral("requestId")).toString().trimmed();
    const QString streamId = root.value(QStringLiteral("streamId")).toString().trimmed();
    const bool configChanged = root.value(QStringLiteral("configChanged")).toBool();
    const bool success = root.value(QStringLiteral("success")).toBool();

    addStreamStatusText_ = message.isEmpty()
        ? (success
            ? QStringLiteral("RTSP 流添加成功")
            : QStringLiteral("RTSP 流添加失败"))
        : message;
    emit addStreamStatusTextChanged();

    QString requestedUrl;
    if (!requestId.isEmpty()) {
        QMutexLocker locker(&streamsMutex_);
        requestedUrl = pendingRequestUrls_.take(requestId);
    }

    if (success && action == QStringLiteral("add_stream") && !streamId.isEmpty() && !requestedUrl.isEmpty()) {
        QMutexLocker locker(&streamsMutex_);
        streamPullUrls_.insert(streamId, requestedUrl);
    }

    if (success && configChanged) {
        resetAllStreamStates();
    }

    if (success && action == QStringLiteral("remove_stream") && !streamId.isEmpty()) {
        removeStreamState(streamId);
        return;
    }

    if (success && action == QStringLiteral("add_stream") && !streamId.isEmpty()) {
        bool streamAlreadyVisible = false;
        {
            QMutexLocker locker(&streamsMutex_);
            preferredStreamId_ = streamId;
            streamAlreadyVisible = streams_.contains(streamId);
        }

        if (streamAlreadyVisible) {
            setSelectedStreamId(streamId);
        }
    }
}

void RtspPlayerController::removeStreamState(const QString& streamId) {
    const QString normalizedId = streamId.trimmed();
    if (normalizedId.isEmpty()) {
        return;
    }

    std::shared_ptr<StreamState> removedStream;
    QString fallbackStreamId;
    bool removedSelected = false;

    {
        QMutexLocker locker(&streamsMutex_);
        const auto it = streams_.find(normalizedId);
        if (it == streams_.end()) {
            return;
        }

        removedStream = it.value();
        streams_.erase(it);
        removedSelected = selectedStreamId_ == normalizedId;
        if (preferredStreamId_ == normalizedId) {
            preferredStreamId_.clear();
        }
        streamPullUrls_.remove(normalizedId);
        if (removedSelected) {
            selectedStreamId_.clear();
            QStringList remainingIds = streams_.keys();
            std::sort(remainingIds.begin(), remainingIds.end());
            if (!remainingIds.isEmpty()) {
                fallbackStreamId = remainingIds.front();
            }
        }
    }

    if (removedStream != nullptr) {
        removedStream->stop.store(true, std::memory_order_release);
        removedStream->queueCv.notify_all();
        if (removedStream->worker.joinable()) {
            removedStream->worker.join();
        }
        destroyDecoder(removedStream.get());
    }

    emit streamsChanged();
    emit statusTextChanged();
    emit selectedStreamInfoChanged();

    if (removedSelected) {
        emit selectedStreamIdChanged();
        emit frameCleared();
        if (!fallbackStreamId.isEmpty()) {
            setSelectedStreamId(fallbackStreamId);
        }
    }
}

QString RtspPlayerController::pullUrlForStream(const QString& streamId) const {
    const QString normalizedId = streamId.trimmed();
    if (normalizedId.isEmpty()) {
        return QString();
    }

    QMutexLocker locker(&streamsMutex_);
    return streamPullUrls_.value(normalizedId);
}

std::shared_ptr<RtspPlayerController::StreamState> RtspPlayerController::streamState(const QString& streamId) const {
    const QString normalizedId = streamId.trimmed();
    if (normalizedId.isEmpty()) {
        return nullptr;
    }

    QMutexLocker locker(&streamsMutex_);
    return streams_.value(normalizedId);
}

std::shared_ptr<RtspPlayerController::StreamState> RtspPlayerController::ensureStreamState(
    const QString& streamId,
    const QString& topic) {
    const QString normalizedId = streamId.trimmed();
    if (normalizedId.isEmpty()) {
        return nullptr;
    }

    std::shared_ptr<StreamState> stream;
    bool created = false;
    bool shouldAutoSelect = false;

    {
        QMutexLocker locker(&streamsMutex_);
        stream = streams_.value(normalizedId);
        if (stream == nullptr) {
            stream = std::make_shared<StreamState>();
            stream->streamId = normalizedId;
            stream->topic = topic.trimmed();
            stream->fpsWindowStartMs = nowMs();
            streams_.insert(normalizedId, stream);
            created = true;
            const QString preferredId = preferredStreamId_.trimmed();
            if (!preferredId.isEmpty() && preferredId == normalizedId) {
                shouldAutoSelect = true;
                selectedStreamId_ = normalizedId;
                preferredStreamId_.clear();
            } else if (selectedStreamId_.trimmed().isEmpty()) {
                shouldAutoSelect = true;
                selectedStreamId_ = normalizedId;
            }
        } else if (stream->topic.trimmed().isEmpty()) {
            stream->topic = topic.trimmed();
        }
    }

    if (created) {
        stream->worker = std::thread([this, stream]() {
            streamWorkerLoop(stream);
        });

        QMetaObject::invokeMethod(this, [this, shouldAutoSelect]() {
            emit streamsChanged();
            if (shouldAutoSelect) {
                emit selectedStreamIdChanged();
            }
            emit statusTextChanged();
            emit selectedStreamInfoChanged();
        }, Qt::QueuedConnection);
    }

    return stream;
}

void RtspPlayerController::stopAllStreams() {
    QList<std::shared_ptr<StreamState>> allStreams;
    {
        QMutexLocker locker(&streamsMutex_);
        allStreams = streams_.values();
        streams_.clear();
    }

    for (const auto& stream : allStreams) {
        if (stream == nullptr) {
            continue;
        }
        stream->stop.store(true, std::memory_order_release);
        stream->queueCv.notify_all();
    }

    for (const auto& stream : allStreams) {
        if (stream != nullptr && stream->worker.joinable()) {
            stream->worker.join();
        }
        if (stream != nullptr) {
            destroyDecoder(stream.get());
        }
    }
}

void RtspPlayerController::streamWorkerLoop(const std::shared_ptr<StreamState>& stream) {
    if (stream == nullptr) {
        return;
    }

    while (!stream->stop.load(std::memory_order_acquire)) {
        EncodedPacket packet;
        bool hasPacket = false;
        bool resetPending = false;

        {
            std::unique_lock<std::mutex> lock(stream->queueMutex);
            stream->queueCv.wait(lock, [&]() {
                return stream->stop.load(std::memory_order_acquire)
                    || stream->resetPending.load(std::memory_order_acquire)
                    || !stream->queue.empty();
            });

            if (stream->stop.load(std::memory_order_acquire)) {
                break;
            }

            resetPending = stream->resetPending.exchange(false, std::memory_order_acq_rel);

            if (!preferPacketIntegrity_.load(std::memory_order_acquire)) {
                const qint64 currentMs = nowMs();
                while (!stream->queue.empty()) {
                    const qint64 wallClockUs = stream->queue.front().payload.wallClockUs;
                    if (wallClockUs <= 0) {
                        break;
                    }

                    const qint64 packetAgeMs = currentMs - wallClockUs / 1000;
                    if (packetAgeMs <= kTargetLatencyMs) {
                        break;
                    }

                    stream->queuedBytes -= static_cast<qint64>(stream->queue.front().payload.encoded.size())
                        + static_cast<qint64>(stream->queue.front().payload.extradata.size());
                    stream->queue.pop_front();
                    stream->queuedPacketCount.fetch_sub(1, std::memory_order_acq_rel);
                    stream->droppedPacketCount.fetch_add(1, std::memory_order_acq_rel);
                    resetPending = true;
                }
            }

            if (!stream->queue.empty()) {
                packet = std::move(stream->queue.front());
                stream->queuedBytes -= static_cast<qint64>(packet.payload.encoded.size())
                    + static_cast<qint64>(packet.payload.extradata.size());
                stream->queue.pop_front();
                stream->queuedPacketCount.fetch_sub(1, std::memory_order_acq_rel);
                hasPacket = true;
            }
        }

        if (resetPending) {
            stream->waitingForKeyFrame.store(true, std::memory_order_release);
            resetDecoder(stream.get());
        }

        if (!hasPacket) {
            if (!shouldDecodeStream(stream->streamId) && stream->decodeActive) {
                destroyDecoder(stream.get());
                stream->decodeActive = false;
                stream->waitingForKeyFrame.store(true, std::memory_order_release);
                stream->fps.store(0, std::memory_order_release);
                stream->fpsWindowFrames = 0;
                stream->fpsWindowStartMs = 0;
            }
            continue;
        }

        const bool decodeCurrentStream = shouldDecodeStream(stream->streamId);
        if (!decodeCurrentStream) {
            if (stream->decodeActive) {
                destroyDecoder(stream.get());
                stream->decodeActive = false;
                stream->waitingForKeyFrame.store(true, std::memory_order_release);
                stream->fps.store(0, std::memory_order_release);
                stream->fpsWindowFrames = 0;
                stream->fpsWindowStartMs = 0;
            }
            continue;
        }

        stream->decodeActive = true;

        if (!preferPacketIntegrity_.load(std::memory_order_acquire) && packet.payload.wallClockUs > 0) {
            const qint64 packetAgeMs = nowMs() - packet.payload.wallClockUs / 1000;
            if (packetAgeMs > kTargetLatencyMs) {
                stream->droppedPacketCount.fetch_add(1, std::memory_order_acq_rel);
                stream->waitingForKeyFrame.store(true, std::memory_order_release);
                resetDecoder(stream.get());
                continue;
            }
        }

        if (!ensureDecoder(stream.get(), packet)) {
            continue;
        }

        if (stream->waitingForKeyFrame.load(std::memory_order_acquire)
            && (packet.payload.flags & AV_PKT_FLAG_KEY) == 0) {
            continue;
        }

        bool producedFrame = false;
        QImage image = decodePacket(stream.get(), packet, &producedFrame);
        if (!producedFrame || image.isNull()) {
            continue;
        }

        stream->waitingForKeyFrame.store(false, std::memory_order_release);
        stream->decodedFrameCount.fetch_add(1, std::memory_order_acq_rel);

        const qint64 currentMs = nowMs();
        ++stream->fpsWindowFrames;
        if (stream->fpsWindowStartMs <= 0) {
            stream->fpsWindowStartMs = currentMs;
        }
        const qint64 elapsedMs = currentMs - stream->fpsWindowStartMs;
        if (elapsedMs >= kFpsWindowMs) {
            const int fps = elapsedMs > 0
                ? static_cast<int>(static_cast<double>(stream->fpsWindowFrames) * 1000.0 / static_cast<double>(elapsedMs))
                : 0;
            stream->fps.store(fps, std::memory_order_release);
            stream->fpsWindowFrames = 0;
            stream->fpsWindowStartMs = currentMs;
        }

        {
            QMutexLocker locker(&stream->lastFrameMutex);
            stream->lastFrame = image;
        }

        if (stream->streamId == selectedStreamId()) {
            queueFramePresentation(stream->streamId, image);
        }
    }
}

bool RtspPlayerController::ensureDecoder(StreamState* stream, const EncodedPacket& packet) {
    if (stream == nullptr) {
        return false;
    }

    const int packetCodecId = packet.payload.codecId;
    const QByteArray packetExtradata = copyExtradata(packet.payload.extradata);
    const bool needsRebuild =
        stream->codecContext == nullptr
        || stream->codecContext->codec_id != static_cast<AVCodecID>(packetCodecId)
        || stream->decoderExtradata != packetExtradata;

    if (!needsRebuild) {
        return true;
    }

    destroyDecoder(stream);

    const AVCodec* codec = avcodec_find_decoder(static_cast<AVCodecID>(packetCodecId));
    if (codec == nullptr) {
        qWarning() << "[RTSP_PLAYER] 未找到解码器，codecId=" << packetCodecId;
        return false;
    }

    stream->codecContext = avcodec_alloc_context3(codec);
    if (stream->codecContext == nullptr) {
        qWarning() << "[RTSP_PLAYER] avcodec_alloc_context3 失败";
        return false;
    }

    if (!packetExtradata.isEmpty()) {
        stream->codecContext->extradata = static_cast<uint8_t*>(av_mallocz(
            static_cast<size_t>(packetExtradata.size()) + AV_INPUT_BUFFER_PADDING_SIZE));
        if (stream->codecContext->extradata == nullptr) {
            qWarning() << "[RTSP_PLAYER] extradata 分配失败";
            destroyDecoder(stream);
            return false;
        }

        std::memcpy(
            stream->codecContext->extradata,
            packetExtradata.constData(),
            static_cast<size_t>(packetExtradata.size()));
        stream->codecContext->extradata_size = packetExtradata.size();
    }

    stream->codecContext->thread_count = 1;
#ifdef FF_THREAD_SLICE
    stream->codecContext->thread_type = FF_THREAD_SLICE;
#endif
    stream->codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
#ifdef AV_CODEC_FLAG2_FAST
    stream->codecContext->flags2 |= AV_CODEC_FLAG2_FAST;
#endif
    stream->codecContext->skip_loop_filter = AVDISCARD_NONREF;

    if (avcodec_open2(stream->codecContext, codec, nullptr) < 0) {
        qWarning() << "[RTSP_PLAYER] avcodec_open2 失败";
        destroyDecoder(stream);
        return false;
    }

    stream->decodedFrame = av_frame_alloc();
    stream->packet = av_packet_alloc();
    if (stream->decodedFrame == nullptr || stream->packet == nullptr) {
        qWarning() << "[RTSP_PLAYER] 解码帧或包分配失败";
        destroyDecoder(stream);
        return false;
    }

    stream->decoderExtradata = packetExtradata;
    stream->codecId.store(packetCodecId, std::memory_order_release);
    stream->waitingForKeyFrame.store(true, std::memory_order_release);
    return true;
}

void RtspPlayerController::resetDecoder(StreamState* stream) {
    if (stream == nullptr) {
        return;
    }

    if (stream->codecContext != nullptr) {
        avcodec_flush_buffers(stream->codecContext);
    }
}

void RtspPlayerController::destroyDecoder(StreamState* stream) {
    if (stream == nullptr) {
        return;
    }

    if (stream->packet != nullptr) {
        av_packet_free(&stream->packet);
    }
    if (stream->decodedFrame != nullptr) {
        av_frame_free(&stream->decodedFrame);
    }
    if (stream->codecContext != nullptr) {
        avcodec_free_context(&stream->codecContext);
    }
    if (stream->swsContext != nullptr) {
        sws_freeContext(stream->swsContext);
        stream->swsContext = nullptr;
    }

    stream->swsSrcW = 0;
    stream->swsSrcH = 0;
    stream->swsSrcFormat = AV_PIX_FMT_NONE;
    stream->swsFlags = 0;
    stream->decoderExtradata.clear();
    stream->frameWidth.store(0, std::memory_order_release);
    stream->frameHeight.store(0, std::memory_order_release);
}

QImage RtspPlayerController::decodePacket(StreamState* stream, const EncodedPacket& packet, bool* producedFrame) {
    if (producedFrame != nullptr) {
        *producedFrame = false;
    }

    if (stream == nullptr || stream->codecContext == nullptr || stream->decodedFrame == nullptr || stream->packet == nullptr) {
        return QImage();
    }

    av_packet_unref(stream->packet);
    if (av_new_packet(stream->packet, packet.payload.encoded.size()) < 0) {
        return QImage();
    }

    std::memcpy(
        stream->packet->data,
        packet.payload.encoded.constData(),
        static_cast<size_t>(packet.payload.encoded.size()));
    stream->packet->pts = packet.payload.pts;
    stream->packet->dts = packet.payload.dts;
    stream->packet->duration = packet.payload.duration;
    stream->packet->flags = packet.payload.flags;
    stream->packet->stream_index = packet.payload.streamIndex;

    int ret = avcodec_send_packet(stream->codecContext, stream->packet);
    if (ret < 0) {
        stream->waitingForKeyFrame.store(true, std::memory_order_release);
        resetDecoder(stream);
        return QImage();
    }

    QImage output;

    while (ret >= 0) {
        ret = avcodec_receive_frame(stream->codecContext, stream->decodedFrame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            stream->waitingForKeyFrame.store(true, std::memory_order_release);
            resetDecoder(stream);
            return QImage();
        }

        const int width = stream->decodedFrame->width;
        const int height = stream->decodedFrame->height;
        const AVPixelFormat srcFormat = static_cast<AVPixelFormat>(stream->decodedFrame->format);
        const int desiredSwsFlags = swsFlagsForCurrentMode();

        if (width <= 0 || height <= 0) {
            continue;
        }

        if (stream->swsContext == nullptr
            || stream->swsSrcW != width
            || stream->swsSrcH != height
            || stream->swsSrcFormat != srcFormat
            || stream->swsFlags != desiredSwsFlags) {
            if (stream->swsContext != nullptr) {
                sws_freeContext(stream->swsContext);
                stream->swsContext = nullptr;
            }

            stream->swsContext = sws_getContext(
                width,
                height,
                srcFormat,
                width,
                height,
                AV_PIX_FMT_BGRA,
                desiredSwsFlags,
                nullptr,
                nullptr,
                nullptr);

            if (stream->swsContext == nullptr) {
                return QImage();
            }

            stream->swsSrcW = width;
            stream->swsSrcH = height;
            stream->swsSrcFormat = srcFormat;
            stream->swsFlags = desiredSwsFlags;
        }

        QImage image(width, height, QImage::Format_ARGB32);
        if (image.isNull()) {
            return QImage();
        }

        uint8_t* dstData[4] = {
            image.bits(),
            nullptr,
            nullptr,
            nullptr
        };
        int dstLineSize[4] = {
            image.bytesPerLine(),
            0,
            0,
            0
        };

        sws_scale(
            stream->swsContext,
            stream->decodedFrame->data,
            stream->decodedFrame->linesize,
            0,
            height,
            dstData,
            dstLineSize);

        stream->frameWidth.store(width, std::memory_order_release);
        stream->frameHeight.store(height, std::memory_order_release);
        output = image;
        if (producedFrame != nullptr) {
            *producedFrame = true;
        }
    }

    return output;
}

void RtspPlayerController::queueFramePresentation(const QString& streamId, const QImage& image) {
    if (image.isNull()) {
        return;
    }

    bool shouldSchedule = false;
    {
        QMutexLocker locker(&frameMutex_);
        pendingFrameStreamId_ = streamId;
        pendingFrame_ = image;
        if (!frameDispatchPending_) {
            frameDispatchPending_ = true;
            shouldSchedule = true;
        }
    }
    if (shouldSchedule) {
        QMetaObject::invokeMethod(this, [this]() {
            deliverPendingFrame();
        }, Qt::QueuedConnection);
    }
}

void RtspPlayerController::deliverPendingFrame() {
    QImage image;
    QString streamId;

    {
        QMutexLocker locker(&frameMutex_);
        if (!frameDispatchPending_ || pendingFrame_.isNull()) {
            return;
        }
        image = pendingFrame_;
        streamId = pendingFrameStreamId_;
        pendingFrame_ = QImage();
        pendingFrameStreamId_.clear();
        frameDispatchPending_ = false;
    }

    if (!image.isNull()) {
        emit framePresented(image, streamId);
    }
}

void RtspPlayerController::refreshViewState() {
    updatePresentationTimerInterval();
    emit streamsChanged();
    emit statusTextChanged();
    emit selectedStreamInfoChanged();
}

void RtspPlayerController::updatePresentationTimerInterval() {
    int targetIntervalMs = kDefaultPresentationIntervalMs;

    const auto currentStream = streamState(selectedStreamId());
    if (currentStream != nullptr) {
        const int fps = currentStream->fps.load(std::memory_order_acquire);
        if (fps > 0) {
            targetIntervalMs = std::clamp(
                static_cast<int>(std::lround(1000.0 / static_cast<double>(fps))),
                kMinPresentationIntervalMs,
                kMaxPresentationIntervalMs);
        }
    }

    if (framePresentationIntervalMs_ == targetIntervalMs) {
        return;
    }

    framePresentationIntervalMs_ = targetIntervalMs;
    if (framePresentTimer_ != nullptr) {
        framePresentTimer_->setInterval(framePresentationIntervalMs_);
    }
}

bool RtspPlayerController::parseReplayActiveEvent(const QByteArray& payload, bool* replayActive) {
    if (replayActive == nullptr) {
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const QString event = root.value(QStringLiteral("event")).toString();
    if (event == QStringLiteral("replay_started")) {
        *replayActive = true;
        return true;
    }
    if (event == QStringLiteral("replay_stopped")) {
        *replayActive = false;
        return true;
    }
    return false;
}

QString RtspPlayerController::codecName(int codecId) {
    if (codecId <= 0) {
        return QStringLiteral("unknown");
    }

    const char* codecNameValue = avcodec_get_name(static_cast<AVCodecID>(codecId));
    if (codecNameValue == nullptr || codecNameValue[0] == '\0') {
        return QStringLiteral("unknown");
    }

    return QString::fromUtf8(codecNameValue);
}

QString RtspPlayerController::formatWallClock(qint64 wallClockUs) {
    if (wallClockUs <= 0) {
        return QStringLiteral("--");
    }

    return QDateTime::fromMSecsSinceEpoch(wallClockUs / 1000).toString(QStringLiteral("HH:mm:ss.zzz"));
}
