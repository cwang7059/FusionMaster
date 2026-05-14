#include "rtsp_replay_plugin.h"

#include <plugin_api/imessage_bus.h>
#include <plugin_api/iplugin_manager.h>
#include <plugin_api/ireplay_runtime_state.h>
#include <plugin_api/rtsp_packet_format.h>

#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QString>
#include <QStringList>

#include <chrono>
#include <thread>

extern "C" {
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
}

namespace {

constexpr auto kRtspReplayControlTopic = "rtsp/replay/control";
constexpr auto kRtspReplayControlResultTopic = "rtsp/replay/control/result";

QString ffmpegErrorToString(int err) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(err, buffer, sizeof(buffer));
    return QString::fromUtf8(buffer);
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

int resolveReconnectMs() {
    bool ok = false;
    int value = qEnvironmentVariableIntValue("OSGI_RTSP_RECONNECT_MS", &ok);
    if (!ok || value < 200) {
        value = 1000;
    }
    return value;
}

bool packetLogEnabled() {
    static const bool enabled = []() {
        const QString value = qEnvironmentVariable("OSGI_RTSP_PACKET_LOG").trimmed().toLower();
        return value == QStringLiteral("1")
            || value == QStringLiteral("true")
            || value == QStringLiteral("yes")
            || value == QStringLiteral("on");
    }();
    return enabled;
}

int interruptRead(void* opaque) {
    if (opaque == nullptr) {
        return 0;
    }
    auto* stopFlag = static_cast<std::atomic<bool>*>(opaque);
    return stopFlag->load(std::memory_order_acquire) ? 1 : 0;
}

QByteArray packPacketPayload(
    const AVPacket& packet,
    const AVStream* stream,
    int codecId,
    const QByteArray& cachedExtradata) {
    plugin_api::RtspPacketPayload rtspPacket;
    rtspPacket.pts = static_cast<qint64>(packet.pts);
    rtspPacket.dts = static_cast<qint64>(packet.dts);
    rtspPacket.duration = static_cast<qint64>(packet.duration);
    rtspPacket.flags = static_cast<qint32>(packet.flags);
    rtspPacket.streamIndex = static_cast<qint32>(packet.stream_index);
    rtspPacket.codecId = static_cast<qint32>(codecId);
    rtspPacket.wallClockUs = QDateTime::currentMSecsSinceEpoch() * 1000;

    if (packet.size > 0 && packet.data != nullptr) {
        rtspPacket.encoded = QByteArray(reinterpret_cast<const char*>(packet.data), packet.size);
    }

    if (!cachedExtradata.isEmpty()) {
        rtspPacket.extradata = cachedExtradata;
    } else if (stream != nullptr
        && stream->codecpar != nullptr
        && stream->codecpar->extradata != nullptr
        && stream->codecpar->extradata_size > 0) {
        rtspPacket.extradata = QByteArray(
            reinterpret_cast<const char*>(stream->codecpar->extradata),
            stream->codecpar->extradata_size);
    }

    return plugin_api::packRtspPacketPayload(rtspPacket);
}

QByteArray packetExtradataFromSideData(const AVPacket& packet) {
    size_t sideDataSize = 0;
    const uint8_t* sideData = av_packet_get_side_data(
        &packet,
        AV_PKT_DATA_NEW_EXTRADATA,
        &sideDataSize);
    if (sideData == nullptr || sideDataSize == 0) {
        return QByteArray();
    }
    return QByteArray(
        reinterpret_cast<const char*>(sideData),
        static_cast<int>(sideDataSize));
}

bool isH26xCodecId(int codecId) {
    return codecId == AV_CODEC_ID_H264 || codecId == AV_CODEC_ID_HEVC;
}

bool parseReplayActiveEvent(const QByteArray& payload, bool* replayActive) {
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

bool parseControlCommand(
    const QByteArray& payload,
    QString* action,
    QString* requestId,
    QString* streamId,
    QString* url) {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const QString parsedAction = root.value(QStringLiteral("action")).toString().trimmed();
    if (parsedAction != QStringLiteral("add_stream")
        && parsedAction != QStringLiteral("remove_stream")) {
        return false;
    }

    if (action != nullptr) {
        *action = parsedAction;
    }
    if (requestId != nullptr) {
        *requestId = root.value(QStringLiteral("requestId")).toString().trimmed();
    }
    if (streamId != nullptr) {
        *streamId = root.value(QStringLiteral("streamId")).toString().trimmed();
    }
    if (url != nullptr) {
        *url = root.value(QStringLiteral("url")).toString().trimmed();
    }

    return true;
}

}  // namespace

void RtspReplayPlugin::start() {
    qInfo() << "[RTSP_REPLAY] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[RTSP_REPLAY] PluginManager 不可用";
        return;
    }

    messageBus_ = manager->messageBus();
    if (messageBus_ == nullptr) {
        qWarning() << "[RTSP_REPLAY] IMessageBus 不可用，RTSP 拉流消息不会发布";
    }

    replayActive_.store(false, std::memory_order_release);
    replayStatusSubscriptionId_ = 0;
    controlSubscriptionId_ = 0;

    if (auto* replayState = manager->getService<IReplayRuntimeState>(QStringLiteral("replay_runtime_state"))) {
        replayActive_.store(replayState->replayActive(), std::memory_order_release);
    }

    if (messageBus_ != nullptr) {
        replayStatusSubscriptionId_ = messageBus_->subscribeWithTopic(
            QStringLiteral("record_replay/status"),
            [this](const QString&, const QByteArray& payload) {
                bool replayActive = false;
                if (!parseReplayActiveEvent(payload, &replayActive)) {
                    return;
                }

                const bool previous = replayActive_.exchange(replayActive, std::memory_order_acq_rel);
                if (previous == replayActive) {
                    return;
                }

                if (replayActive) {
                    qInfo() << "[RTSP_REPLAY] 进入重演状态，停止 RTSP 拉流";
                    stopWorkers();
                } else {
                    qInfo() << "[RTSP_REPLAY] 离开重演状态，恢复 RTSP 拉流";
                    startWorkers();
                }
            });

        controlSubscriptionId_ = messageBus_->subscribeWithTopicRaw(
            QString::fromLatin1(kRtspReplayControlTopic),
            [this](const QString& topic, const QByteArray& payload) {
                if (topic != QString::fromLatin1(kRtspReplayControlTopic)) {
                    return;
                }
                handleControlCommand(payload);
            });
    }

    controllerMarker_ = new QObject();
    manager->registerService<QObject>(
        QStringLiteral("controller/%1").arg(name()),
        controllerMarker_);

    avformat_network_init();

    if (!replayActive_.load(std::memory_order_acquire)) {
        startWorkers();
    } else {
        qInfo() << "[RTSP_REPLAY] 当前处于重演状态，延迟拉流启动";
    }
}

void RtspReplayPlugin::stop() {
    qInfo() << "[RTSP_REPLAY] 插件停止";

    if (messageBus_ != nullptr && replayStatusSubscriptionId_ != 0) {
        messageBus_->unsubscribe(replayStatusSubscriptionId_);
    }
    replayStatusSubscriptionId_ = 0;
    if (messageBus_ != nullptr && controlSubscriptionId_ != 0) {
        messageBus_->unsubscribe(controlSubscriptionId_);
    }
    controlSubscriptionId_ = 0;

    stopWorkers();
    replayActive_.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
        dynamicConfigs_.clear();
        removedStreamIds_.clear();
    }

    avformat_network_deinit();
    delete controllerMarker_;
    controllerMarker_ = nullptr;
    messageBus_ = nullptr;
}

QString RtspReplayPlugin::name() const {
    return QStringLiteral("org_common_rtsp_replay");
}

void RtspReplayPlugin::startWorkers() {
    std::lock_guard<std::mutex> guard(workersMutex_);
    if (!workers_.empty()) {
        return;
    }
    if (replayActive_.load(std::memory_order_acquire)) {
        return;
    }

    const std::vector<StreamConfig> configs = loadConfigs();
    if (configs.empty()) {
        qInfo() << "[RTSP_REPLAY] 未配置 OSGI_RTSP_URLS，插件保持空闲";
        return;
    }

    for (const StreamConfig& config : configs) {
        auto worker = std::make_unique<StreamWorker>();
        worker->config = config;
        worker->thread = std::thread([this, raw = worker.get()]() {
            runStreamWorker(raw);
        });
        workers_.push_back(std::move(worker));
    }

    qInfo() << "[RTSP_REPLAY] 已启动流数量:" << static_cast<int>(workers_.size());
}

void RtspReplayPlugin::stopWorkers() {
    std::vector<std::unique_ptr<StreamWorker>> workers;
    {
        std::lock_guard<std::mutex> guard(workersMutex_);
        if (workers_.empty()) {
            return;
        }
        workers.swap(workers_);
    }

    for (const auto& worker : workers) {
        if (worker != nullptr) {
            worker->stop.store(true, std::memory_order_release);
        }
    }

    for (const auto& worker : workers) {
        if (worker != nullptr && worker->thread.joinable()) {
            worker->thread.join();
        }
    }
}

std::vector<RtspReplayPlugin::StreamConfig> RtspReplayPlugin::loadConfigs() const {
    std::vector<StreamConfig> result = loadEnvConfigs();
    std::vector<StreamConfig> dynamicConfigs;
    QSet<QString> removedStreamIds;
    {
        std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
        dynamicConfigs = dynamicConfigs_;
        removedStreamIds = removedStreamIds_;
    }

    result.erase(
        std::remove_if(
            result.begin(),
            result.end(),
            [&](const StreamConfig& config) {
                return removedStreamIds.contains(config.streamId);
            }),
        result.end());

    for (const StreamConfig& config : dynamicConfigs) {
        if (!removedStreamIds.contains(config.streamId)) {
            result.push_back(config);
        }
    }
    return result;
}

std::vector<RtspReplayPlugin::StreamConfig> RtspReplayPlugin::loadEnvConfigs() const {
    std::vector<StreamConfig> result;

    const QString raw = qEnvironmentVariable("OSGI_RTSP_URLS").trimmed();
    if (raw.isEmpty()) {
        return result;
    }

    const QStringList tokens = splitBySemicolon(raw);
    if (tokens.isEmpty()) {
        return result;
    }

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
            qWarning().noquote() << QStringLiteral("[RTSP_REPLAY] 忽略非法 URL: %1").arg(token);
            continue;
        }

        const QString normalizedId = makeUniqueStreamId(streamId, usedIds, index);
        usedIds.insert(normalizedId);

        StreamConfig config;
        config.streamId = normalizedId;
        config.url = url;
        config.topic = QStringLiteral("rtsp/raw/%1").arg(normalizedId);
        result.push_back(config);

        ++index;
    }

    return result;
}

void RtspReplayPlugin::restartWorkers() {
    if (replayActive_.load(std::memory_order_acquire)) {
        return;
    }

    stopWorkers();
    startWorkers();
}

void RtspReplayPlugin::handleControlCommand(const QByteArray& payload) {
    QString action;
    QString requestId;
    QString streamId;
    QString url;
    if (!parseControlCommand(payload, &action, &requestId, &streamId, &url)) {
        return;
    }

    QString effectiveStreamId;
    QString message;
    bool configChanged = false;
    bool success = false;
    if (action == QStringLiteral("add_stream")) {
        success = addDynamicStream(
            streamId,
            url,
            &effectiveStreamId,
            &message,
            &configChanged);
    } else if (action == QStringLiteral("remove_stream")) {
        success = removeStream(
            streamId,
            &effectiveStreamId,
            &message,
            &configChanged);
    }

    if (success && configChanged && !replayActive_.load(std::memory_order_acquire)) {
        restartWorkers();
    }

    publishControlResult(action, requestId, success, message, configChanged, effectiveStreamId);
}

bool RtspReplayPlugin::addDynamicStream(
    const QString& requestedStreamId,
    const QString& url,
    QString* effectiveStreamId,
    QString* message,
    bool* configChanged) {
    const QString trimmedUrl = url.trimmed();
    if (!isRtspUrl(trimmedUrl)) {
        if (message != nullptr) {
            *message = QStringLiteral("RTSP 地址无效，只支持 rtsp:// 或 rtsps://");
        }
        if (configChanged != nullptr) {
            *configChanged = false;
        }
        return false;
    }

    const std::vector<StreamConfig> envConfigs = loadEnvConfigs();
    QSet<QString> usedIds;
    for (const StreamConfig& config : envConfigs) {
        usedIds.insert(config.streamId);
        if (config.url.compare(trimmedUrl, Qt::CaseInsensitive) == 0) {
            if (effectiveStreamId != nullptr) {
                *effectiveStreamId = config.streamId;
            }
            const bool wasRemoved = [&]() {
                std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
                return removedStreamIds_.contains(config.streamId);
            }();
            if (wasRemoved) {
                std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
                removedStreamIds_.remove(config.streamId);
                if (message != nullptr) {
                    *message = replayActive_.load(std::memory_order_acquire)
                        ? QStringLiteral("已恢复流 %1，当前处于重演模式，退出重演后自动启动").arg(config.streamId)
                        : QStringLiteral("已恢复流 %1，正在启动拉流").arg(config.streamId);
                }
                if (configChanged != nullptr) {
                    *configChanged = true;
                }
            } else {
                if (message != nullptr) {
                    *message = QStringLiteral("该 RTSP 地址已经在环境配置中，复用流 %1").arg(config.streamId);
                }
                if (configChanged != nullptr) {
                    *configChanged = false;
                }
            }
            return true;
        }
    }

    std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
    for (const StreamConfig& config : dynamicConfigs_) {
        usedIds.insert(config.streamId);
        if (config.url.compare(trimmedUrl, Qt::CaseInsensitive) == 0) {
            if (effectiveStreamId != nullptr) {
                *effectiveStreamId = config.streamId;
            }
            if (message != nullptr) {
                *message = QStringLiteral("该 RTSP 地址已经添加过，复用流 %1").arg(config.streamId);
            }
            if (configChanged != nullptr) {
                *configChanged = false;
            }
            return true;
        }
    }

    const QString normalizedId =
        makeUniqueStreamId(requestedStreamId, usedIds, static_cast<int>(envConfigs.size() + dynamicConfigs_.size()));

    StreamConfig config;
    config.streamId = normalizedId;
    config.url = trimmedUrl;
    config.topic = QStringLiteral("rtsp/raw/%1").arg(normalizedId);
    dynamicConfigs_.push_back(config);

    if (effectiveStreamId != nullptr) {
        *effectiveStreamId = normalizedId;
    }
    if (configChanged != nullptr) {
        *configChanged = true;
    }

    if (message != nullptr) {
        if (replayActive_.load(std::memory_order_acquire)) {
            *message = QStringLiteral("已添加流 %1，当前处于重演模式，退出重演后自动启动").arg(normalizedId);
        } else {
            *message = QStringLiteral("已添加流 %1，正在启动拉流").arg(normalizedId);
        }
    }

    qInfo().noquote()
        << QStringLiteral("[RTSP_REPLAY] 动态添加流: %1 -> %2")
               .arg(config.streamId, config.url);

    return true;
}

bool RtspReplayPlugin::removeStream(
    const QString& streamId,
    QString* effectiveStreamId,
    QString* message,
    bool* configChanged) {
    const QString normalizedId = streamId.trimmed();
    if (normalizedId.isEmpty()) {
        if (message != nullptr) {
            *message = QStringLiteral("未选择要删除的实时流");
        }
        if (configChanged != nullptr) {
            *configChanged = false;
        }
        return false;
    }

    if (effectiveStreamId != nullptr) {
        *effectiveStreamId = normalizedId;
    }

    const std::vector<StreamConfig> envConfigs = loadEnvConfigs();
    for (const StreamConfig& config : envConfigs) {
        if (config.streamId != normalizedId) {
            continue;
        }

        std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
        if (removedStreamIds_.contains(normalizedId)) {
            if (message != nullptr) {
                *message = QStringLiteral("实时流 %1 已删除").arg(normalizedId);
            }
            if (configChanged != nullptr) {
                *configChanged = false;
            }
            return true;
        }

        removedStreamIds_.insert(normalizedId);
        if (message != nullptr) {
            *message = replayActive_.load(std::memory_order_acquire)
                ? QStringLiteral("已删除实时流 %1，本次运行内退出重演后也不会恢复").arg(normalizedId)
                : QStringLiteral("已删除实时流 %1").arg(normalizedId);
        }
        if (configChanged != nullptr) {
            *configChanged = true;
        }
        qInfo().noquote() << QStringLiteral("[RTSP_REPLAY] 删除环境流: %1").arg(normalizedId);
        return true;
    }

    std::lock_guard<std::mutex> guard(dynamicConfigsMutex_);
    const auto it = std::find_if(
        dynamicConfigs_.begin(),
        dynamicConfigs_.end(),
        [&](const StreamConfig& config) {
            return config.streamId == normalizedId;
        });
    if (it == dynamicConfigs_.end()) {
        if (message != nullptr) {
            *message = QStringLiteral("未找到可删除的实时流 %1").arg(normalizedId);
        }
        if (configChanged != nullptr) {
            *configChanged = false;
        }
        return false;
    }

    dynamicConfigs_.erase(it);
    removedStreamIds_.remove(normalizedId);
    if (message != nullptr) {
        *message = QStringLiteral("已删除实时流 %1").arg(normalizedId);
    }
    if (configChanged != nullptr) {
        *configChanged = true;
    }

    qInfo().noquote() << QStringLiteral("[RTSP_REPLAY] 删除动态流: %1").arg(normalizedId);
    return true;
}

void RtspReplayPlugin::publishControlResult(
    const QString& action,
    const QString& requestId,
    bool success,
    const QString& message,
    bool configChanged,
    const QString& effectiveStreamId) const {
    if (messageBus_ == nullptr) {
        return;
    }

    QJsonObject root;
    root.insert(QStringLiteral("action"), action);
    root.insert(QStringLiteral("requestId"), requestId);
    root.insert(QStringLiteral("success"), success);
    root.insert(QStringLiteral("message"), message);
    root.insert(QStringLiteral("configChanged"), configChanged);
    root.insert(QStringLiteral("streamId"), effectiveStreamId);
    messageBus_->publish(
        QString::fromLatin1(kRtspReplayControlResultTopic),
        QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void RtspReplayPlugin::runStreamWorker(StreamWorker* worker) {
    if (worker == nullptr) {
        return;
    }

    const int reconnectMs = resolveReconnectMs();
    const QString streamId = worker->config.streamId;
    const QString topic = worker->config.topic;
    const QString url = worker->config.url;

    qInfo().noquote() << QStringLiteral("[RTSP_REPLAY] [流%1] 启动: %2").arg(streamId, url);

    while (!worker->stop.load(std::memory_order_acquire)) {
        AVFormatContext* formatContext = avformat_alloc_context();
        if (formatContext == nullptr) {
            qWarning().noquote() << QStringLiteral("[RTSP_REPLAY] [流%1] avformat_alloc_context 失败").arg(streamId);
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnectMs));
            continue;
        }

        formatContext->interrupt_callback.callback = interruptRead;
        formatContext->interrupt_callback.opaque = &worker->stop;

        AVDictionary* options = nullptr;
        av_dict_set(&options, "rtsp_transport", "tcp", 0);
        av_dict_set(&options, "stimeout", "3000000", 0);
        av_dict_set(&options, "rw_timeout", "5000000", 0);
        av_dict_set(&options, "max_delay", "500000", 0);
        av_dict_set(&options, "buffer_size", "800000", 0);
        av_dict_set(&options, "reorder_queue_size", "0", 0);
        av_dict_set(&options, "fflags", "nobuffer+discardcorrupt", 0);
        av_dict_set(&options, "flags", "low_delay", 0);
        av_dict_set(&options, "probesize", "2000000", 0);
        av_dict_set(&options, "analyzeduration", "1000000", 0);

        const int openRet = avformat_open_input(&formatContext, url.toUtf8().constData(), nullptr, &options);
        av_dict_free(&options);

        if (openRet < 0) {
            qWarning().noquote()
                << QStringLiteral("[RTSP_REPLAY] [流%1] 打开失败: %2")
                       .arg(streamId, ffmpegErrorToString(openRet));
            if (formatContext != nullptr) {
                avformat_free_context(formatContext);
                formatContext = nullptr;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnectMs));
            continue;
        }

        const int infoRet = avformat_find_stream_info(formatContext, nullptr);
        if (infoRet < 0) {
            qWarning().noquote()
                << QStringLiteral("[RTSP_REPLAY] [流%1] 获取流信息失败: %2")
                       .arg(streamId, ffmpegErrorToString(infoRet));
            avformat_close_input(&formatContext);
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnectMs));
            continue;
        }

        const int videoStreamIndex = av_find_best_stream(
            formatContext,
            AVMEDIA_TYPE_VIDEO,
            -1,
            -1,
            nullptr,
            0);

        if (videoStreamIndex < 0) {
            qWarning().noquote() << QStringLiteral("[RTSP_REPLAY] [流%1] 未找到视频流").arg(streamId);
            avformat_close_input(&formatContext);
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnectMs));
            continue;
        }

        AVStream* stream = formatContext->streams[videoStreamIndex];
        const int codecId = stream != nullptr && stream->codecpar != nullptr
            ? static_cast<int>(stream->codecpar->codec_id)
            : static_cast<int>(AV_CODEC_ID_NONE);
        QByteArray cachedExtradata;
        if (stream != nullptr
            && stream->codecpar != nullptr
            && stream->codecpar->extradata != nullptr
            && stream->codecpar->extradata_size > 0) {
            cachedExtradata = QByteArray(
                reinterpret_cast<const char*>(stream->codecpar->extradata),
                stream->codecpar->extradata_size);
        }
        bool warnedNoExtradata = false;

        AVPacket* packet = av_packet_alloc();
        if (packet == nullptr) {
            qWarning().noquote() << QStringLiteral("[RTSP_REPLAY] [流%1] AVPacket 分配失败").arg(streamId);
            avformat_close_input(&formatContext);
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnectMs));
            continue;
        }

        qInfo().noquote()
            << QStringLiteral("[RTSP_REPLAY] [流%1] 已连接，topic=%2").arg(streamId, topic);

        while (!worker->stop.load(std::memory_order_acquire)) {
            const int readRet = av_read_frame(formatContext, packet);
            if (readRet < 0) {
                if (readRet != AVERROR_EOF && readRet != AVERROR_EXIT) {
                    qWarning().noquote()
                        << QStringLiteral("[RTSP_REPLAY] [流%1] 读包失败: %2")
                               .arg(streamId, ffmpegErrorToString(readRet));
                }
                break;
            }

            if (packet->stream_index == videoStreamIndex && messageBus_ != nullptr) {
                const QByteArray sideExtradata = packetExtradataFromSideData(*packet);
                if (!sideExtradata.isEmpty()) {
                    cachedExtradata = sideExtradata;
                }

                if (isH26xCodecId(codecId) && cachedExtradata.isEmpty() && !warnedNoExtradata) {
                    warnedNoExtradata = true;
                    qWarning().noquote()
                        << QStringLiteral("[RTSP_REPLAY] [流%1] H26x 未获取到 SPS/PPS(extradata)，等待后续关键帧/side-data")
                               .arg(streamId);
                }

                const QByteArray payload = packPacketPayload(*packet, stream, codecId, cachedExtradata);
                messageBus_->publish(topic, payload);

                if (packetLogEnabled()) {
                    qDebug().noquote()
                        << QStringLiteral("[RTSP_REPLAY] topic=%1 pts=%2 size=%3")
                               .arg(topic)
                               .arg(packet->pts)
                               .arg(packet->size);
                }
            }

            av_packet_unref(packet);
        }

        av_packet_free(&packet);
        avformat_close_input(&formatContext);

        if (!worker->stop.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnectMs));
        }
    }

    qInfo().noquote() << QStringLiteral("[RTSP_REPLAY] [流%1] 停止").arg(streamId);
}
