#include "record_replay_controller.h"

#include <plugin_api/rtsp_packet_format.h>

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDesktopServices>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaObject>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QThread>
#include <QUrl>

#include <algorithm>
#include <chrono>
#include <cmath>

namespace {

constexpr quint32 kRecordFrameMagic = 0x52524631u;  // "RRF1"
constexpr qint64 kRecordFrameHeaderSize = 24;
constexpr quint32 kRecordFrameMaxFieldBytes = 64u * 1024u * 1024u;
constexpr const char* kChunkExtBinary = ".recbin";
constexpr const char* kRtspTimelineFile = "rtsp_timeline.tsv";
constexpr const char* kRtspIndexFile = "rtsp_index.json";

struct DecodedFrame {
    qint64 timestampUs = 0;
    QString topic;
    QByteArray payload;
    QString source;
};

qint64 nowUs() {
    return QDateTime::currentDateTimeUtc().toMSecsSinceEpoch() * 1000;
}

QString nativePath(const QString& path) {
    return QDir::toNativeSeparators(QDir::cleanPath(path));
}

QString newSessionId() {
    return QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd.HHmmss"));
}

QString chunkFileNameByIndex(int chunkIndex) {
    return QStringLiteral("chunk_%1%2")
        .arg(chunkIndex, 4, 10, QChar('0'))
        .arg(QLatin1String(kChunkExtBinary));
}

bool writeBinaryFrame(QFile* file, const DecodedFrame& frame, qint64* frameBytes) {
    if (file == nullptr) return false;

    const QByteArray topicBytes = frame.topic.toUtf8();
    const QByteArray sourceBytes = frame.source.toUtf8();
    const quint32 topicLen = static_cast<quint32>(topicBytes.size());
    const quint32 sourceLen = static_cast<quint32>(sourceBytes.size());
    const quint32 payloadLen = static_cast<quint32>(frame.payload.size());

    QDataStream stream(file);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << static_cast<quint32>(kRecordFrameMagic);
    stream << static_cast<qint64>(frame.timestampUs);
    stream << topicLen;
    stream << sourceLen;
    stream << payloadLen;
    if (topicLen > 0 && stream.writeRawData(topicBytes.constData(), static_cast<int>(topicLen)) != static_cast<int>(topicLen)) return false;
    if (sourceLen > 0 && stream.writeRawData(sourceBytes.constData(), static_cast<int>(sourceLen)) != static_cast<int>(sourceLen)) return false;
    if (payloadLen > 0 && stream.writeRawData(frame.payload.constData(), static_cast<int>(payloadLen)) != static_cast<int>(payloadLen)) return false;
    if (stream.status() != QDataStream::Ok) return false;

    if (frameBytes != nullptr) {
        *frameBytes = kRecordFrameHeaderSize + static_cast<qint64>(topicLen) + static_cast<qint64>(sourceLen) + static_cast<qint64>(payloadLen);
    }
    return true;
}

enum class ReadFrameResult {
    Ok,
    Eof,
    Corrupt,
};

ReadFrameResult readBinaryFrame(QFile* file, DecodedFrame* frame, qint64* frameBytes) {
    if (file == nullptr || frame == nullptr) return ReadFrameResult::Corrupt;
    if (file->atEnd()) return ReadFrameResult::Eof;

    QDataStream stream(file);
    stream.setByteOrder(QDataStream::LittleEndian);

    quint32 magic = 0;
    qint64 timestampUs = 0;
    quint32 topicLen = 0;
    quint32 sourceLen = 0;
    quint32 payloadLen = 0;
    stream >> magic;
    if (stream.status() == QDataStream::ReadPastEnd) return ReadFrameResult::Eof;
    stream >> timestampUs;
    stream >> topicLen;
    stream >> sourceLen;
    stream >> payloadLen;
    if (stream.status() != QDataStream::Ok) return ReadFrameResult::Corrupt;
    if (magic != kRecordFrameMagic) return ReadFrameResult::Corrupt;
    if (topicLen > kRecordFrameMaxFieldBytes || sourceLen > kRecordFrameMaxFieldBytes || payloadLen > kRecordFrameMaxFieldBytes) {
        return ReadFrameResult::Corrupt;
    }

    QByteArray topicBytes(static_cast<int>(topicLen), Qt::Uninitialized);
    QByteArray sourceBytes(static_cast<int>(sourceLen), Qt::Uninitialized);
    QByteArray payloadBytes(static_cast<int>(payloadLen), Qt::Uninitialized);
    if (topicLen > 0 && stream.readRawData(topicBytes.data(), static_cast<int>(topicLen)) != static_cast<int>(topicLen)) return ReadFrameResult::Corrupt;
    if (sourceLen > 0 && stream.readRawData(sourceBytes.data(), static_cast<int>(sourceLen)) != static_cast<int>(sourceLen)) return ReadFrameResult::Corrupt;
    if (payloadLen > 0 && stream.readRawData(payloadBytes.data(), static_cast<int>(payloadLen)) != static_cast<int>(payloadLen)) return ReadFrameResult::Corrupt;
    if (stream.status() != QDataStream::Ok) return ReadFrameResult::Corrupt;

    frame->timestampUs = timestampUs;
    frame->topic = QString::fromUtf8(topicBytes);
    frame->source = QString::fromUtf8(sourceBytes);
    frame->payload = std::move(payloadBytes);
    if (frameBytes != nullptr) {
        *frameBytes = kRecordFrameHeaderSize + static_cast<qint64>(topicLen) + static_cast<qint64>(sourceLen) + static_cast<qint64>(payloadLen);
    }
    return ReadFrameResult::Ok;
}

bool parseRtspWallClockUs(const QByteArray& payload, qint64* wallClockUs) {
    if (wallClockUs == nullptr) return false;
    plugin_api::RtspPacketPayload packet;
    if (!plugin_api::parseRtspPacketPayload(payload, &packet)) return false;
    if (packet.wallClockUs <= 0) return false;
    *wallClockUs = packet.wallClockUs;
    return true;
}

}  // namespace

RecordReplayController::RecordReplayController(QObject* parent)
    : QObject(parent) {
    recordTargetDir_ = defaultRecordTargetDir();
    runtimeTicker_ = new QTimer(this);
    runtimeTicker_->setInterval(250);
    connect(runtimeTicker_, &QTimer::timeout, this, [this]() {
        if (recording() || replaying()) {
            emit runtimeChanged();
        }
    });
    runtimeTicker_->start();
    refreshDiskSummary();
    refreshSessionList();
}

RecordReplayController::~RecordReplayController() {
    stopReplay();
    stopRecord();
    stopWriterThread(false);
    resetRtspIndexState();
}

void RecordReplayController::setMessageBus(IMessageBus* messageBus) {
    messageBus_ = messageBus;
}

void RecordReplayController::onBusMessage(const QString& topic, const QByteArray& payload, const QString& source) {
    if (!recording_.load(std::memory_order_acquire) || !shouldRecordTopic(topic)) {
        return;
    }

    qint64 timestampUs = nowUs();
    if (topic.startsWith(QStringLiteral("rtsp/raw/"))) {
        qint64 rtspWallClockUs = 0;
        if (parseRtspWallClockUs(payload, &rtspWallClockUs)) {
            timestampUs = rtspWallClockUs;
        }
    }

    RecordFrame frame;
    frame.timestampUs = timestampUs;
    frame.topic = topic;
    frame.payload = payload;
    frame.source = source.trimmed().isEmpty() ? QStringLiteral("runtime") : source.trimmed();

    const qint64 bytes = 24 + frame.topic.toUtf8().size() + frame.source.toUtf8().size() + frame.payload.size();
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (queuedBytes_ + bytes > kQueueLimitBytes) {
            ++droppedFrameCount_;
            return;
        }
        queue_.push_back(std::move(frame));
        queuedBytes_ += bytes;
    }
    queueCv_.notify_one();
}

bool RecordReplayController::recording() const { return recording_.load(std::memory_order_acquire); }
bool RecordReplayController::replaying() const { return replaying_.load(std::memory_order_acquire); }
double RecordReplayController::replayProgress() const {
    const double value = replayProgress_.load(std::memory_order_acquire);
    return std::max(0.0, std::min(1.0, value));
}
double RecordReplayController::replaySpeed() const { return replaySpeed_.load(std::memory_order_acquire); }
QString RecordReplayController::replayProgressText() const {
    const int percent = static_cast<int>(replayProgress() * 100.0 + 0.5);
    if (!replaying() && percent <= 0) return QStringLiteral("未开始");
    return QStringLiteral("%1%").arg(percent);
}
QString RecordReplayController::operationFeedback() const {
    QMutexLocker locker(&stateMutex_);
    return operationFeedback_;
}
QString RecordReplayController::topicFilterText() const { return topicFilterText_; }
QString RecordReplayController::activeRecordFile() const { return activeRecordFile_; }
QString RecordReplayController::replayStartText() const { return replayStartText_; }
QString RecordReplayController::replayEndText() const { return replayEndText_; }
QString RecordReplayController::replayModeText() const { return QStringLiteral("当前文件"); }

QString RecordReplayController::recordingStateText() const {
    if (replaying()) return QStringLiteral("重演中");
    if (recording()) return QStringLiteral("记录中");
    return QStringLiteral("未开始");
}

QString RecordReplayController::recordingElapsedText() const {
    if (!recording()) return QStringLiteral("0:00:00");
    return formatDuration(std::max<qint64>(0, QDateTime::currentMSecsSinceEpoch() - recordStartMs_));
}

QString RecordReplayController::replayElapsedText() const {
    const bool active = replaying();
    const double progress = replayProgress_.load(std::memory_order_acquire);
    if (!active && progress <= 0.0) return QStringLiteral("0:00:00");

    const qint64 beginUs = replayRangeBeginUs_.load(std::memory_order_acquire);
    const qint64 endUs = replayRangeEndUs_.load(std::memory_order_acquire);
    qint64 currentUs = replayCurrentUs_.load(std::memory_order_acquire);
    if (endUs > beginUs) {
        if (currentUs < beginUs) currentUs = beginUs;
        if (currentUs > endUs) currentUs = endUs;
    }
    return formatDuration(std::max<qint64>(0, (currentUs - beginUs) / 1000));
}

QString RecordReplayController::diskSummary() const {
    QMutexLocker locker(&stateMutex_);
    return diskSummary_;
}

bool RecordReplayController::rtspAvailable() const {
    QMutexLocker locker(&stateMutex_);
    return rtspAvailable_;
}

void RecordReplayController::setRtspAvailable(bool available) {
    bool changed = false;
    {
        QMutexLocker locker(&stateMutex_);
        if (rtspAvailable_ != available) {
            rtspAvailable_ = available;
            changed = true;
        }
    }
    if (changed) {
        emit rtspAvailableChanged();
    }
}

void RecordReplayController::setTopicFilterText(const QString& text) {
    const QString value = text.trimmed();
    if (value == topicFilterText_) return;
    topicFilterText_ = value;
    emit topicFilterTextChanged();
}

QString RecordReplayController::recordTargetDir() const {
    return nativePath(recordTargetDir_);
}

void RecordReplayController::setRecordTargetDir(const QString& dirPath) {
    const QString path = QDir::cleanPath(dirPath.trimmed());
    if (path.isEmpty() || path == recordTargetDir_) return;
    recordTargetDir_ = path;
    emit recordTargetDirChanged();
    refreshDiskSummary();
    refreshSessionList();
}

QVariantList RecordReplayController::sessionList() const {
    QMutexLocker locker(&stateMutex_);
    return sessionList_;
}

int RecordReplayController::selectedSessionIndex() const {
    QMutexLocker locker(&stateMutex_);
    return selectedSessionIndex_;
}

QVariantMap RecordReplayController::selectedSessionInfo() const {
    QMutexLocker locker(&stateMutex_);
    return selectedSessionInfo_;
}

void RecordReplayController::setSelectedSessionIndex(int index) {
    QVariantMap info;
    {
        QMutexLocker locker(&stateMutex_);
        if (index < 0 || index >= sessionList_.size()) index = -1;
        if (selectedSessionIndex_ == index) return;
        selectedSessionIndex_ = index;
        selectedSessionInfo_ = index >= 0 ? sessionList_.at(index).toMap() : QVariantMap();
        info = selectedSessionInfo_;
    }
    replayStartText_ = info.value(QStringLiteral("startTime")).toString();
    replayEndText_ = info.value(QStringLiteral("endTime")).toString();
    emit selectedSessionIndexChanged();
    emit selectedSessionInfoChanged();
    emit replayRangeChanged();
}

void RecordReplayController::setReplayStartText(const QString& text) {
    const QString value = text.trimmed();
    if (value == replayStartText_) return;
    replayStartText_ = value;
    emit replayRangeChanged();
}

void RecordReplayController::setReplayEndText(const QString& text) {
    const QString value = text.trimmed();
    if (value == replayEndText_) return;
    replayEndText_ = value;
    emit replayRangeChanged();
}

QString RecordReplayController::chooseRecordDirectory() {
    return QFileDialog::getExistingDirectory(
        nullptr,
        QStringLiteral("选择记录目录"),
        recordTargetDir(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

void RecordReplayController::refreshSessionList() {
    QDir root(defaultRecordRootDir());
    if (!root.exists()) root.mkpath(QStringLiteral("."));

    QString selectedDir;
    {
        QMutexLocker locker(&stateMutex_);
        selectedDir = selectedSessionInfo_.value(QStringLiteral("directory")).toString();
    }

    std::vector<SessionData> sessions;
    const QFileInfoList dirs = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo& info : dirs) {
        SessionData session = loadSession(info.absoluteFilePath());
        if (!session.chunks.isEmpty()) sessions.push_back(std::move(session));
    }
    std::sort(sessions.begin(), sessions.end(), [](const SessionData& a, const SessionData& b) {
        return a.startUs > b.startUs;
    });

    QVariantList rows;
    for (const SessionData& session : sessions) rows.push_back(toSessionInfo(session));

    int nextIndex = -1;
    QVariantMap nextInfo;
    if (!rows.isEmpty()) {
        for (int i = 0; i < rows.size(); ++i) {
            const QVariantMap row = rows.at(i).toMap();
            if (!selectedDir.isEmpty() && row.value(QStringLiteral("directory")).toString() == selectedDir) {
                nextIndex = i;
                nextInfo = row;
                break;
            }
        }
        if (nextIndex < 0) {
            nextIndex = 0;
            nextInfo = rows.first().toMap();
        }
    }

    {
        QMutexLocker locker(&stateMutex_);
        sessionList_ = rows;
        selectedSessionIndex_ = nextIndex;
        selectedSessionInfo_ = nextInfo;
    }
    replayStartText_ = nextInfo.value(QStringLiteral("startTime")).toString();
    replayEndText_ = nextInfo.value(QStringLiteral("endTime")).toString();

    emit sessionListChanged();
    emit selectedSessionIndexChanged();
    emit selectedSessionInfoChanged();
    emit replayRangeChanged();
}

bool RecordReplayController::startRecord() {
    if (recording() || replaying()) {
        setOperationFeedback(QStringLiteral("启动记录失败：当前正在记录或重演"));
        return false;
    }

    QDir root(defaultRecordRootDir());
    if (!root.exists() && !root.mkpath(QStringLiteral("."))) {
        setOperationFeedback(QStringLiteral("启动记录失败：无法创建目录 %1").arg(nativePath(root.absolutePath())));
        return false;
    }

    activeSession_ = SessionData();
    activeSession_.sessionId = newSessionId();
    activeSession_.directory = root.filePath(activeSession_.sessionId);
    if (!QDir(activeSession_.directory).exists() && !root.mkpath(activeSession_.sessionId)) {
        setOperationFeedback(QStringLiteral("启动记录失败：无法创建会话目录"));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.clear();
        queuedBytes_ = 0;
    }
    droppedFrameCount_ = 0;
    replayPublishedCount_ = 0;
    currentChunkIndex_ = 0;
    currentChunkBytes_ = 0;
    resetRtspIndexState();

    if (!openNextChunk()) {
        activeSession_ = SessionData();
        setOperationFeedback(QStringLiteral("启动记录失败：无法创建分块文件"));
        return false;
    }
    if (!openRtspTimelineFile()) {
        qWarning() << "[RECORD_REPLAY] open rtsp timeline file failed";
    }

    recordStartMs_ = QDateTime::currentMSecsSinceEpoch();
    recording_.store(true, std::memory_order_release);
    startWriterThread();

    publishStatus(QStringLiteral("record_started"), QVariantMap{
        {QStringLiteral("sessionId"), activeSession_.sessionId},
        {QStringLiteral("directory"), nativePath(activeSession_.directory)}
    });
    setOperationFeedback(QStringLiteral("记录已开始：%1").arg(activeSession_.sessionId));
    invokeRuntimeChanged();
    return true;
}

void RecordReplayController::stopRecord() {
    if (!recording()) {
        setOperationFeedback(QStringLiteral("当前未在记录"));
        return;
    }

    recording_.store(false, std::memory_order_release);
    stopWriterThread(true);
    closeCurrentChunk();
    const QString completedSessionDir = activeSession_.directory;
    const qint64 recordedCount = activeSession_.recordCount;
    const bool hasData = activeSession_.recordCount > 0 && !activeSession_.chunks.isEmpty();
    if (hasData) {
        saveActiveSessionMeta();
    }
    closeRtspTimelineFile(hasData);
    if (!hasData && !activeSession_.directory.trimmed().isEmpty()) {
        QDir(activeSession_.directory).removeRecursively();
    }

    publishStatus(QStringLiteral("record_stopped"), QVariantMap{
        {QStringLiteral("sessionId"), activeSession_.sessionId},
        {QStringLiteral("count"), activeSession_.recordCount},
        {QStringLiteral("bytes"), activeSession_.payloadBytes},
        {QStringLiteral("dropped"), droppedFrameCount_}
    });

    activeSession_ = SessionData();
    activeRecordFile_.clear();
    refreshSessionList();
    if (hasData && !completedSessionDir.trimmed().isEmpty()) {
        selectSessionByDirectory(completedSessionDir);
    }
    if (hasData) {
        setOperationFeedback(QStringLiteral("记录已停止：共 %1 条，丢弃 %2 条").arg(recordedCount).arg(droppedFrameCount_));
    } else {
        setOperationFeedback(QStringLiteral("记录已停止：无有效数据"));
    }
    invokeRuntimeChanged();
}

bool RecordReplayController::startReplay(bool useRange) {
    if (recording() || replaying()) {
        setOperationFeedback(QStringLiteral("启动重演失败：当前正在记录或重演"));
        return false;
    }

    QVariantMap info;
    {
        QMutexLocker locker(&stateMutex_);
        if (selectedSessionIndex_ < 0 || selectedSessionIndex_ >= sessionList_.size()) {
            setOperationFeedback(QStringLiteral("启动重演失败：未选择会话"));
            return false;
        }
        info = sessionList_.at(selectedSessionIndex_).toMap();
    }

    const QString sessionDir = QDir::fromNativeSeparators(info.value(QStringLiteral("directory")).toString());
    if (sessionDir.trimmed().isEmpty()) {
        setOperationFeedback(QStringLiteral("启动重演失败：会话目录为空"));
        return false;
    }
    SessionData session = loadSession(sessionDir);
    if (session.chunks.isEmpty()) {
        setOperationFeedback(QStringLiteral("启动重演失败：会话没有可重演分块"));
        return false;
    }

    qint64 effectiveBeginUs = session.startUs;
    qint64 effectiveEndUs = session.endUs;
    qint64 displayBeginUs = effectiveBeginUs;
    qint64 displayEndUs = effectiveEndUs;
    if (useRange) {
        qint64 parsedBeginUs = 0;
        qint64 parsedEndUs = 0;
        const bool beginOk = parseRangeTime(replayStartText_, &parsedBeginUs);
        const bool endOk = parseRangeTime(replayEndText_, &parsedEndUs);
        if (!beginOk || !endOk) {
            setOperationFeedback(QStringLiteral("启动重演失败：指定时间段格式无效"));
            return false;
        }
        displayBeginUs = parsedBeginUs;
        displayEndUs = parsedEndUs;
        if (displayBeginUs > displayEndUs) std::swap(displayBeginUs, displayEndUs);
        if (displayEndUs < session.startUs || displayBeginUs > session.endUs) {
            setOperationFeedback(QStringLiteral("启动重演失败：指定时间段不在会话范围内"));
            return false;
        }
        effectiveBeginUs = std::max(displayBeginUs, session.startUs);
        effectiveEndUs = std::min(displayEndUs, session.endUs);
        if (effectiveBeginUs > effectiveEndUs) {
            setOperationFeedback(QStringLiteral("启动重演失败：指定时间段无有效数据"));
            return false;
        }
    }

    replayPublishedCount_ = 0;
    replayProgress_.store(0.0, std::memory_order_release);
    replayCurrentUs_.store(displayBeginUs, std::memory_order_release);
    replayRangeBeginUs_.store(displayBeginUs, std::memory_order_release);
    replayRangeEndUs_.store(displayEndUs, std::memory_order_release);
    startReplayThread(std::move(session), useRange,
                      effectiveBeginUs, effectiveEndUs,
                      displayBeginUs, displayEndUs);
    return true;
}

void RecordReplayController::stopReplay() {
    if (!replaying() && !replayThread_.joinable()) {
        setOperationFeedback(QStringLiteral("当前未在重演"));
        return;
    }
    stopReplayThread();
    replaying_.store(false, std::memory_order_release);
    setOperationFeedback(QStringLiteral("重演已停止：已发布 %1 条").arg(replayPublishedCount_));
    invokeRuntimeChanged();
}

void RecordReplayController::selectPreviousSession() { setSelectedSessionIndex(selectedSessionIndex() - 1); }
void RecordReplayController::selectNextSession() { setSelectedSessionIndex(selectedSessionIndex() + 1); }

void RecordReplayController::decreaseReplaySpeed() {
    static const std::vector<double> speeds = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
    const double current = replaySpeed_.load(std::memory_order_acquire);
    for (int i = static_cast<int>(speeds.size()) - 1; i >= 0; --i) {
        if (speeds[static_cast<size_t>(i)] < current - 0.001) {
            replaySpeed_.store(speeds[static_cast<size_t>(i)], std::memory_order_release);
            emit replaySpeedChanged();
            return;
        }
    }
}

void RecordReplayController::increaseReplaySpeed() {
    static const std::vector<double> speeds = {0.25, 0.5, 1.0, 2.0, 4.0, 8.0};
    const double current = replaySpeed_.load(std::memory_order_acquire);
    for (double speed : speeds) {
        if (speed > current + 0.001) {
            replaySpeed_.store(speed, std::memory_order_release);
            emit replaySpeedChanged();
            return;
        }
    }
}

bool RecordReplayController::seekReplayByProgress(double progress) {
    if (!std::isfinite(progress)) {
        return false;
    }

    const double normalized = std::max(0.0, std::min(1.0, progress));
    const qint64 beginUs = replayRangeBeginUs_.load(std::memory_order_acquire);
    const qint64 endUs = replayRangeEndUs_.load(std::memory_order_acquire);
    const qint64 totalUs = std::max<qint64>(0, endUs - beginUs);
    const qint64 targetUs = beginUs + static_cast<qint64>(static_cast<double>(totalUs) * normalized);

    replayCurrentUs_.store(targetUs, std::memory_order_release);
    replayProgress_.store(normalized, std::memory_order_release);
    invokeRuntimeChanged();

    if (replaying()) {
        replaySeekRequestUs_.store(targetUs, std::memory_order_release);
        replaySleepCv_.notify_all();
        setOperationFeedback(QStringLiteral("重演定位：%1").arg(formatDateTime(targetUs)));
        return true;
    }

    setOperationFeedback(QStringLiteral("定位预览：%1").arg(formatDateTime(targetUs)));
    return true;
}

bool RecordReplayController::removeSelectedSession() {
    if (recording() || replaying()) {
        setOperationFeedback(QStringLiteral("删除失败：记录或重演进行中"));
        return false;
    }

    QVariantMap info;
    {
        QMutexLocker locker(&stateMutex_);
        if (selectedSessionIndex_ < 0 || selectedSessionIndex_ >= sessionList_.size()) {
            setOperationFeedback(QStringLiteral("删除失败：未选择会话"));
            return false;
        }
        info = sessionList_.at(selectedSessionIndex_).toMap();
    }

    const QString sessionDir = QDir::fromNativeSeparators(info.value(QStringLiteral("directory")).toString());
    if (sessionDir.isEmpty()) {
        setOperationFeedback(QStringLiteral("删除失败：会话目录无效"));
        return false;
    }
    const QString rootDir = QDir::cleanPath(QDir::fromNativeSeparators(defaultRecordRootDir()));
    const QString normalizedSessionDir = QDir::cleanPath(QDir::fromNativeSeparators(sessionDir));
    if (normalizedSessionDir == rootDir) {
        setOperationFeedback(QStringLiteral("删除失败：会话目录无效"));
        return false;
    }

    QDir dir(sessionDir);
    if (!dir.exists()) {
        refreshSessionList();
        setOperationFeedback(QStringLiteral("删除失败：会话目录不存在"));
        return false;
    }

    const bool ok = dir.removeRecursively();
    if (ok) {
        publishStatus(QStringLiteral("session_removed"), QVariantMap{{QStringLiteral("directory"), nativePath(sessionDir)}});
        refreshSessionList();
        setOperationFeedback(QStringLiteral("已删除会话：%1").arg(info.value(QStringLiteral("sessionId")).toString()));
    } else {
        setOperationFeedback(QStringLiteral("删除失败：无法删除目录 %1").arg(nativePath(sessionDir)));
    }
    return ok;
}

bool RecordReplayController::openSelectedSessionDirectory() {
    QVariantMap info;
    {
        QMutexLocker locker(&stateMutex_);
        if (selectedSessionIndex_ < 0 || selectedSessionIndex_ >= sessionList_.size()) {
            setOperationFeedback(QStringLiteral("打开目录失败：未选择会话"));
            return false;
        }
        info = sessionList_.at(selectedSessionIndex_).toMap();
    }

    const QString sessionDir = QDir::cleanPath(QDir::fromNativeSeparators(info.value(QStringLiteral("directory")).toString()));
    if (sessionDir.trimmed().isEmpty()) {
        setOperationFeedback(QStringLiteral("打开目录失败：会话目录无效"));
        return false;
    }

    QFileInfo dirInfo(sessionDir);
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        refreshSessionList();
        setOperationFeedback(QStringLiteral("打开目录失败：目录不存在"));
        return false;
    }

    const bool ok = QDesktopServices::openUrl(QUrl::fromLocalFile(dirInfo.absoluteFilePath()));
    if (ok) {
        setOperationFeedback(QStringLiteral("已打开目录：%1").arg(nativePath(dirInfo.absoluteFilePath())));
    } else {
        setOperationFeedback(QStringLiteral("打开目录失败：%1").arg(nativePath(dirInfo.absoluteFilePath())));
    }
    return ok;
}

bool RecordReplayController::clipSelectedRange() {
    if (recording() || replaying()) {
        setOperationFeedback(QStringLiteral("截取失败：记录或重演进行中"));
        return false;
    }

    QVariantMap info;
    {
        QMutexLocker locker(&stateMutex_);
        if (selectedSessionIndex_ < 0 || selectedSessionIndex_ >= sessionList_.size()) {
            setOperationFeedback(QStringLiteral("截取失败：未选择会话"));
            return false;
        }
        info = sessionList_.at(selectedSessionIndex_).toMap();
    }

    const QString sourceDir = QDir::fromNativeSeparators(info.value(QStringLiteral("directory")).toString());
    if (sourceDir.trimmed().isEmpty()) {
        setOperationFeedback(QStringLiteral("截取失败：会话目录无效"));
        return false;
    }

    SessionData source = loadSession(sourceDir);
    if (source.chunks.isEmpty()) {
        setOperationFeedback(QStringLiteral("截取失败：会话没有可截取分块"));
        return false;
    }

    qint64 beginUs = source.startUs;
    qint64 endUs = source.endUs;

    qint64 parsed = 0;
    if (parseRangeTime(replayStartText_, &parsed)) beginUs = parsed;
    if (parseRangeTime(replayEndText_, &parsed)) endUs = parsed;
    if (beginUs > endUs) std::swap(beginUs, endUs);

    QDir root(defaultRecordRootDir());
    if (!root.exists() && !root.mkpath(QStringLiteral("."))) {
        setOperationFeedback(QStringLiteral("截取失败：无法创建根目录"));
        return false;
    }

    SessionData clip;
    clip.sessionId = QStringLiteral("%1_clip_%2")
        .arg(source.sessionId, QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd.HHmmss")));
    clip.directory = root.filePath(clip.sessionId);

    if (!QDir(clip.directory).exists() && !root.mkpath(clip.sessionId)) {
        setOperationFeedback(QStringLiteral("截取失败：无法创建输出目录"));
        return false;
    }

    int chunkIndex = 0;
    qint64 chunkBytes = 0;
    QFile* outFile = nullptr;
    QFile clipRtspTimelineFile(QDir(clip.directory).filePath(rtspTimelineFileName()));
    qint64 clipRtspFrameCount = 0;
    QMap<QString, RtspTopicSummary> clipRtspSummary;
    const bool clipRtspTimelineEnabled =
        clipRtspTimelineFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    if (clipRtspTimelineEnabled) {
        const QByteArray header =
            "timestampUs\tchunk\tchunkOffset\tframeBytes\ttopic\tsource\tpayloadBytes\n";
        clipRtspTimelineFile.write(header);
    }

    auto closeChunk = [&]() {
        if (outFile == nullptr) return;
        outFile->flush();
        outFile->close();
        if (chunkBytes == 0) {
            QFile::remove(outFile->fileName());
            if (!clip.chunks.isEmpty()) clip.chunks.removeLast();
        }
        delete outFile;
        outFile = nullptr;
        chunkBytes = 0;
    };

    auto openChunk = [&]() -> bool {
        closeChunk();
        ++chunkIndex;
        const QString fileName = chunkFileNameByIndex(chunkIndex);
        const QString filePath = QDir(clip.directory).filePath(fileName);
        outFile = new QFile(filePath);
        if (!outFile->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            delete outFile;
            outFile = nullptr;
            return false;
        }
        clip.chunks.push_back(fileName);
        chunkBytes = 0;
        return true;
    };

    auto writeMeta = [&](const SessionData& session, qint64 rtspFrameCount) {
        QJsonObject rootObj;
        rootObj.insert(QStringLiteral("sessionId"), session.sessionId);
        rootObj.insert(QStringLiteral("recordCount"), session.recordCount);
        rootObj.insert(QStringLiteral("payloadBytes"), session.payloadBytes);
        rootObj.insert(QStringLiteral("startUs"), session.startUs);
        rootObj.insert(QStringLiteral("endUs"), session.endUs);
        rootObj.insert(QStringLiteral("startTime"), formatDateTime(session.startUs));
        rootObj.insert(QStringLiteral("endTime"), formatDateTime(session.endUs));
        rootObj.insert(QStringLiteral("rtspFrameCount"), rtspFrameCount);
        if (rtspFrameCount > 0) {
            rootObj.insert(QStringLiteral("rtspTimelineFile"), rtspTimelineFileName());
            rootObj.insert(QStringLiteral("rtspIndexFile"), rtspIndexFileName());
        }
        QJsonArray chunks;
        for (const QString& chunk : session.chunks) chunks.push_back(chunk);
        rootObj.insert(QStringLiteral("chunks"), chunks);
        QFile metaFile(QDir(session.directory).filePath(QStringLiteral("session.json")));
        if (metaFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            metaFile.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
        }
    };

    bool ok = true;
    for (const QString& chunkName : source.chunks) {
        QFile inFile(QDir(source.directory).filePath(chunkName));
        if (!inFile.open(QIODevice::ReadOnly)) continue;

        while (true) {
            DecodedFrame frame;
            ReadFrameResult readResult = readBinaryFrame(&inFile, &frame, nullptr);
            if (readResult == ReadFrameResult::Eof) break;
            if (readResult == ReadFrameResult::Corrupt) {
                ok = false;
                break;
            }
            if (frame.topic.trimmed().isEmpty()) continue;
            if (frame.timestampUs < beginUs || frame.timestampUs > endUs) continue;

            if (outFile == nullptr && !openChunk()) {
                ok = false;
                break;
            }
            if (chunkBytes > 0 && chunkBytes >= kChunkSizeBytes && !openChunk()) {
                ok = false;
                break;
            }

            const qint64 chunkOffset = outFile->pos();
            qint64 writtenBytes = 0;
            if (!writeBinaryFrame(outFile, frame, &writtenBytes)) {
                ok = false;
                break;
            }

            ++clip.recordCount;
            clip.payloadBytes += frame.payload.size();
            if (clip.startUs == 0) clip.startUs = frame.timestampUs;
            clip.endUs = frame.timestampUs;
            chunkBytes += writtenBytes;

            if (clipRtspTimelineEnabled && frame.topic.startsWith(QStringLiteral("rtsp/raw/"))) {
                const QString chunkFileName = clip.chunks.isEmpty() ? QString() : clip.chunks.last();
                const QString line = QStringLiteral("%1\t%2\t%3\t%4\t%5\t%6\t%7\n")
                                         .arg(frame.timestampUs)
                                         .arg(sanitizeTsvField(chunkFileName))
                                         .arg(chunkOffset)
                                         .arg(writtenBytes)
                                         .arg(sanitizeTsvField(frame.topic))
                                         .arg(sanitizeTsvField(frame.source))
                                         .arg(frame.payload.size());
                clipRtspTimelineFile.write(line.toUtf8());
                ++clipRtspFrameCount;

                RtspTopicSummary summary = clipRtspSummary.value(frame.topic);
                ++summary.frameCount;
                summary.payloadBytes += frame.payload.size();
                if (summary.startUs == 0) summary.startUs = frame.timestampUs;
                summary.endUs = frame.timestampUs;
                clipRtspSummary.insert(frame.topic, summary);
            }
        }
        if (!ok) break;
    }

    closeChunk();
    if (clipRtspTimelineEnabled) {
        clipRtspTimelineFile.flush();
        clipRtspTimelineFile.close();
    }

    if (!ok || clip.recordCount <= 0) {
        QDir(clip.directory).removeRecursively();
        setOperationFeedback(QStringLiteral("截取失败：时间段内无数据"));
        return false;
    }

    if (clipRtspFrameCount > 0) {
        writeRtspIndexFile(clip.directory, clipRtspFrameCount, clipRtspSummary);
    } else {
        QFile::remove(QDir(clip.directory).filePath(rtspTimelineFileName()));
        QFile::remove(QDir(clip.directory).filePath(rtspIndexFileName()));
    }

    writeMeta(clip, clipRtspFrameCount);
    publishStatus(QStringLiteral("clip_created"), QVariantMap{
        {QStringLiteral("sourceSession"), source.sessionId},
        {QStringLiteral("clipSession"), clip.sessionId},
        {QStringLiteral("count"), clip.recordCount},
        {QStringLiteral("bytes"), clip.payloadBytes}
    });

    refreshSessionList();
    selectSessionByDirectory(clip.directory);
    setOperationFeedback(QStringLiteral("截取完成：%1，共 %2 条").arg(clip.sessionId).arg(clip.recordCount));
    return true;
}

bool RecordReplayController::shouldRecordTopic(const QString& topic) const {
    const QString filter = topicFilterText_.trimmed();
    if (filter.isEmpty() || filter == QStringLiteral("*")) return true;

    const QStringList tokens = filter.split(QRegularExpression(QStringLiteral("[,;\\s]+")), Qt::SkipEmptyParts);
    for (QString token : tokens) {
        token = token.trimmed();
        if (token.isEmpty() || token == QStringLiteral("*")) return true;
        if (token.endsWith(QStringLiteral("*"))) token.chop(1);
        if (!token.isEmpty() && topic.startsWith(token, Qt::CaseInsensitive)) return true;
    }
    return false;
}

bool RecordReplayController::parseRangeTime(const QString& text, qint64* timestampUs) const {
    if (timestampUs == nullptr) return false;
    const QString value = text.trimmed();
    if (value.isEmpty()) return false;
    const QStringList formats = {
        QStringLiteral("yyyy/MM/dd HH:mm:ss"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss"),
        QStringLiteral("yyyy/MM/dd HH:mm:ss.zzz"),
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"),
        QStringLiteral("yyyyMMddHHmmss")
    };
    for (const QString& format : formats) {
        const QDateTime dt = QDateTime::fromString(value, format);
        if (dt.isValid()) {
            *timestampUs = dt.toMSecsSinceEpoch() * 1000;
            return true;
        }
    }
    return false;
}

void RecordReplayController::invokeRuntimeChanged() {
    if (QThread::currentThread() == thread()) {
        emit runtimeChanged();
        return;
    }
    QMetaObject::invokeMethod(this, [this]() { emit runtimeChanged(); }, Qt::QueuedConnection);
}

void RecordReplayController::setOperationFeedback(const QString& text) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, text]() { setOperationFeedback(text); }, Qt::QueuedConnection);
        return;
    }
    bool changed = false;
    {
        QMutexLocker locker(&stateMutex_);
        if (operationFeedback_ != text) {
            operationFeedback_ = text;
            changed = true;
        }
    }
    if (changed) emit runtimeChanged();
}

void RecordReplayController::refreshDiskSummary() {
    QStorageInfo storage(defaultRecordRootDir());
    if (!storage.isValid() || !storage.isReady()) {
        storage = QStorageInfo(QCoreApplication::applicationDirPath());
    }
    const QString text = QStringLiteral("%1 可用，共 %2")
        .arg(humanReadableBytes(storage.bytesAvailable()))
        .arg(humanReadableBytes(storage.bytesTotal()));
    {
        QMutexLocker locker(&stateMutex_);
        diskSummary_ = text;
    }
    emit diskSummaryChanged();
}

void RecordReplayController::publishStatus(const QString& eventName, const QVariantMap& payload) {
    if (messageBus_ == nullptr) return;
    QJsonObject obj = QJsonObject::fromVariantMap(payload);
    obj.insert(QStringLiteral("event"), eventName);
    obj.insert(QStringLiteral("sender"), QStringLiteral("org_common_record_replay"));
    obj.insert(QStringLiteral("tsUs"), static_cast<qint64>(nowUs()));
    messageBus_->publish(QStringLiteral("record_replay/status"), QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QString RecordReplayController::defaultRecordTargetDir() const {
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("record/org_common_record_replay"));
}

QString RecordReplayController::defaultRecordRootDir() const {
    return recordTargetDir_.trimmed().isEmpty() ? defaultRecordTargetDir() : QDir::cleanPath(recordTargetDir_);
}

RecordReplayController::SessionData RecordReplayController::loadSession(const QString& sessionDir) const {
    SessionData session;
    session.directory = QDir::cleanPath(sessionDir);
    session.sessionId = QFileInfo(session.directory).fileName();
    QString rtspIndexFileNameFromMeta = rtspIndexFileName();

    QFile metaFile(QDir(session.directory).filePath(QStringLiteral("session.json")));
    if (metaFile.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(metaFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject root = doc.object();
            session.sessionId = root.value(QStringLiteral("sessionId")).toString(session.sessionId);
            session.recordCount = root.value(QStringLiteral("recordCount")).toVariant().toLongLong();
            session.payloadBytes = root.value(QStringLiteral("payloadBytes")).toVariant().toLongLong();
            session.startUs = root.value(QStringLiteral("startUs")).toVariant().toLongLong();
            session.endUs = root.value(QStringLiteral("endUs")).toVariant().toLongLong();
            session.rtspFrameCount = root.value(QStringLiteral("rtspFrameCount")).toVariant().toLongLong();
            rtspIndexFileNameFromMeta = root.value(QStringLiteral("rtspIndexFile")).toString(rtspIndexFileName());
            const QJsonArray chunks = root.value(QStringLiteral("chunks")).toArray();
            for (const QJsonValue& value : chunks) {
                if (value.isString()) session.chunks.push_back(value.toString());
            }
        }
    }

    if (session.chunks.isEmpty()) {
        const QFileInfoList files = QDir(session.directory).entryInfoList(
            QStringList() << QStringLiteral("chunk_*.recbin"),
            QDir::Files,
            QDir::Name);
        for (const QFileInfo& info : files) session.chunks.push_back(info.fileName());
    }

    QFile rtspIndexFile(QDir(session.directory).filePath(rtspIndexFileNameFromMeta));
    if (rtspIndexFile.open(QIODevice::ReadOnly)) {
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(rtspIndexFile.readAll(), &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject root = doc.object();
            const qint64 indexFrameCount = root.value(QStringLiteral("frameCount")).toVariant().toLongLong();
            if (session.rtspFrameCount <= 0) session.rtspFrameCount = indexFrameCount;

            const QJsonArray topics = root.value(QStringLiteral("topics")).toArray();
            QStringList summaryParts;
            for (const QJsonValue& value : topics) {
                if (!value.isObject()) continue;
                const QJsonObject node = value.toObject();
                const QString topic = node.value(QStringLiteral("topic")).toString();
                if (topic.trimmed().isEmpty()) continue;
                const qint64 frameCount = node.value(QStringLiteral("frameCount")).toVariant().toLongLong();
                const qint64 payloadBytes = node.value(QStringLiteral("payloadBytes")).toVariant().toLongLong();

                QVariantMap row;
                row.insert(QStringLiteral("topic"), topic);
                row.insert(QStringLiteral("frameCount"), frameCount);
                row.insert(QStringLiteral("payloadBytes"), payloadBytes);
                row.insert(QStringLiteral("payloadBytesText"), humanReadableBytes(static_cast<quint64>(std::max<qint64>(0, payloadBytes))));
                row.insert(QStringLiteral("startUs"), node.value(QStringLiteral("startUs")).toVariant().toLongLong());
                row.insert(QStringLiteral("endUs"), node.value(QStringLiteral("endUs")).toVariant().toLongLong());
                row.insert(QStringLiteral("startTime"), node.value(QStringLiteral("startTime")).toString());
                row.insert(QStringLiteral("endTime"), node.value(QStringLiteral("endTime")).toString());
                session.rtspTopicRows.push_back(row);

                summaryParts.push_back(QStringLiteral("%1:%2帧/%3")
                                           .arg(topic)
                                           .arg(frameCount)
                                           .arg(humanReadableBytes(static_cast<quint64>(std::max<qint64>(0, payloadBytes)))));
            }
            session.rtspTopicCount = session.rtspTopicRows.size();
            session.rtspTopicSummaryText = summaryParts.join(QStringLiteral("   |   "));
        }
    }

    return session;
}

QVariantMap RecordReplayController::toSessionInfo(const SessionData& session) const {
    QVariantMap row;
    row.insert(QStringLiteral("sessionId"), session.sessionId);
    row.insert(QStringLiteral("directory"), nativePath(session.directory));
    row.insert(QStringLiteral("recordCount"), session.recordCount);
    row.insert(QStringLiteral("payloadBytes"), session.payloadBytes);
    row.insert(QStringLiteral("payloadBytesText"), humanReadableBytes(static_cast<quint64>(std::max<qint64>(0, session.payloadBytes))));
    row.insert(QStringLiteral("startUs"), session.startUs);
    row.insert(QStringLiteral("endUs"), session.endUs);
    row.insert(QStringLiteral("startTime"), formatDateTime(session.startUs));
    row.insert(QStringLiteral("endTime"), formatDateTime(session.endUs));
    row.insert(QStringLiteral("durationText"), formatDuration((session.endUs - session.startUs) / 1000));
    row.insert(QStringLiteral("chunkCount"), session.chunks.size());
    row.insert(QStringLiteral("rtspFrameCount"), session.rtspFrameCount);
    row.insert(QStringLiteral("rtspTopicCount"), session.rtspTopicCount);
    row.insert(QStringLiteral("rtspTopicRows"), session.rtspTopicRows);
    row.insert(QStringLiteral("rtspTopicSummaryText"), session.rtspTopicSummaryText);
    return row;
}

void RecordReplayController::saveActiveSessionMeta() const {
    if (activeSession_.directory.trimmed().isEmpty()) return;
    QJsonObject root;
    root.insert(QStringLiteral("sessionId"), activeSession_.sessionId);
    root.insert(QStringLiteral("recordCount"), activeSession_.recordCount);
    root.insert(QStringLiteral("payloadBytes"), activeSession_.payloadBytes);
    root.insert(QStringLiteral("startUs"), activeSession_.startUs);
    root.insert(QStringLiteral("endUs"), activeSession_.endUs);
    root.insert(QStringLiteral("startTime"), formatDateTime(activeSession_.startUs));
    root.insert(QStringLiteral("endTime"), formatDateTime(activeSession_.endUs));
    root.insert(QStringLiteral("rtspFrameCount"), rtspTimelineCount_);
    if (rtspTimelineCount_ > 0) {
        root.insert(QStringLiteral("rtspTimelineFile"), rtspTimelineFileName());
        root.insert(QStringLiteral("rtspIndexFile"), rtspIndexFileName());
    }
    QJsonArray chunks;
    for (const QString& chunk : activeSession_.chunks) chunks.push_back(chunk);
    root.insert(QStringLiteral("chunks"), chunks);
    QFile file(QDir(activeSession_.directory).filePath(QStringLiteral("session.json")));
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    }
}

bool RecordReplayController::selectSessionByDirectory(const QString& directory) {
    const QString normalized = QDir::cleanPath(QDir::fromNativeSeparators(directory));
    const QVariantList rows = sessionList();
    for (int i = 0; i < rows.size(); ++i) {
        const QVariantMap row = rows.at(i).toMap();
        const QString rowDir = QDir::cleanPath(QDir::fromNativeSeparators(row.value(QStringLiteral("directory")).toString()));
        if (rowDir == normalized) {
            setSelectedSessionIndex(i);
            return true;
        }
    }
    return false;
}

QString RecordReplayController::formatDateTime(qint64 timestampUs) {
    if (timestampUs <= 0) return QString();
    return QDateTime::fromMSecsSinceEpoch(timestampUs / 1000).toString(QStringLiteral("yyyy/MM/dd HH:mm:ss"));
}

QString RecordReplayController::formatDuration(qint64 milliseconds) {
    if (milliseconds < 0) milliseconds = 0;
    const qint64 sec = milliseconds / 1000;
    return QStringLiteral("%1:%2:%3")
        .arg(sec / 3600)
        .arg((sec % 3600) / 60, 2, 10, QChar('0'))
        .arg(sec % 60, 2, 10, QChar('0'));
}

QString RecordReplayController::humanReadableBytes(quint64 bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = static_cast<double>(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
    return QStringLiteral("%1 %2").arg(QString::number(value, 'f', unit == 0 ? 0 : 1), units[unit]);
}

void RecordReplayController::startWriterThread() {
    stopWriterThread(false);
    writerStop_.store(false, std::memory_order_release);
    writerThread_ = std::thread([this]() { writerLoop(); });
}

void RecordReplayController::stopWriterThread(bool flushQueue) {
    if (!writerThread_.joinable()) return;
    writerStop_.store(true, std::memory_order_release);
    if (!flushQueue) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        queue_.clear();
        queuedBytes_ = 0;
    }
    queueCv_.notify_all();
    writerThread_.join();
}

void RecordReplayController::writerLoop() {
    while (true) {
        RecordFrame frame;
        bool hasFrame = false;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            queueCv_.wait_for(lock, std::chrono::milliseconds(100), [this]() {
                return writerStop_.load(std::memory_order_acquire) || !queue_.empty();
            });
            if (!queue_.empty()) {
                frame = std::move(queue_.front());
                queue_.pop_front();
                const qint64 bytes = 24 + frame.topic.toUtf8().size() + frame.source.toUtf8().size() + frame.payload.size();
                queuedBytes_ = std::max<qint64>(0, queuedBytes_ - bytes);
                hasFrame = true;
            } else if (writerStop_.load(std::memory_order_acquire)) {
                break;
            }
        }
        if (hasFrame) {
            if (!writeFrame(frame)) ++droppedFrameCount_;
            continue;
        }
        if (writerStop_.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(queueMutex_);
            if (queue_.empty()) break;
        }
    }
}

bool RecordReplayController::openNextChunk() {
    closeCurrentChunk();
    ++currentChunkIndex_;
    const QString fileName = chunkFileNameByIndex(currentChunkIndex_);
    const QString filePath = QDir(activeSession_.directory).filePath(fileName);
    currentChunkFile_ = new QFile(filePath);
    if (!currentChunkFile_->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        delete currentChunkFile_;
        currentChunkFile_ = nullptr;
        return false;
    }
    currentChunkBytes_ = 0;
    activeSession_.chunks.push_back(fileName);
    activeRecordFile_ = nativePath(filePath);
    invokeRuntimeChanged();
    return true;
}

void RecordReplayController::closeCurrentChunk() {
    if (currentChunkFile_ == nullptr) return;
    currentChunkFile_->flush();
    currentChunkFile_->close();
    if (currentChunkBytes_ == 0) {
        const QString filePath = currentChunkFile_->fileName();
        QFile::remove(filePath);
        if (!activeSession_.chunks.isEmpty()) activeSession_.chunks.removeLast();
    }
    delete currentChunkFile_;
    currentChunkFile_ = nullptr;
    currentChunkBytes_ = 0;
}

bool RecordReplayController::writeFrame(const RecordFrame& frame) {
    if (currentChunkFile_ == nullptr && !openNextChunk()) return false;
    if (currentChunkBytes_ > 0 && currentChunkBytes_ >= kChunkSizeBytes && !openNextChunk()) return false;

    const qint64 chunkOffset = currentChunkFile_->pos();
    qint64 writtenBytes = 0;
    DecodedFrame decoded;
    decoded.timestampUs = frame.timestampUs;
    decoded.topic = frame.topic;
    decoded.source = frame.source;
    decoded.payload = frame.payload;
    if (!writeBinaryFrame(currentChunkFile_, decoded, &writtenBytes)) return false;

    currentChunkBytes_ += writtenBytes;
    ++activeSession_.recordCount;
    activeSession_.payloadBytes += frame.payload.size();
    if (activeSession_.startUs == 0) activeSession_.startUs = frame.timestampUs;
    activeSession_.endUs = frame.timestampUs;

    if (frame.topic.startsWith(QStringLiteral("rtsp/raw/"))) {
        const QString chunkFileName = activeSession_.chunks.isEmpty() ? QString() : activeSession_.chunks.last();
        appendRtspTimeline(frame, chunkFileName, chunkOffset, writtenBytes);
    }
    return true;
}

bool RecordReplayController::openRtspTimelineFile() {
    if (activeSession_.directory.trimmed().isEmpty()) return false;
    if (rtspTimelineFile_ != nullptr) return true;

    const QString filePath = QDir(activeSession_.directory).filePath(rtspTimelineFileName());
    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        delete file;
        return false;
    }

    const QByteArray header = "timestampUs\tchunk\tchunkOffset\tframeBytes\ttopic\tsource\tpayloadBytes\n";
    if (file->write(header) != header.size()) {
        file->close();
        delete file;
        return false;
    }

    rtspTimelineFile_ = file;
    return true;
}

void RecordReplayController::closeRtspTimelineFile(bool persistBySession) {
    QString timelinePath;
    QString indexPath;
    if (!activeSession_.directory.trimmed().isEmpty()) {
        timelinePath = QDir(activeSession_.directory).filePath(rtspTimelineFileName());
        indexPath = QDir(activeSession_.directory).filePath(rtspIndexFileName());
    }

    if (rtspTimelineFile_ != nullptr) {
        rtspTimelineFile_->flush();
        rtspTimelineFile_->close();
        delete rtspTimelineFile_;
        rtspTimelineFile_ = nullptr;
    }

    if (persistBySession && activeSession_.recordCount > 0 && rtspTimelineCount_ > 0) {
        writeRtspIndexFile(activeSession_.directory, rtspTimelineCount_, rtspTopicSummary_);
    } else {
        if (!timelinePath.isEmpty()) QFile::remove(timelinePath);
        if (!indexPath.isEmpty()) QFile::remove(indexPath);
    }

    rtspTimelineCount_ = 0;
    rtspTopicSummary_.clear();
}

void RecordReplayController::appendRtspTimeline(const RecordFrame& frame,
                                                const QString& chunkFileName,
                                                qint64 chunkOffset,
                                                qint64 frameBytes) {
    if (rtspTimelineFile_ == nullptr && !openRtspTimelineFile()) return;
    if (rtspTimelineFile_ == nullptr) return;

    const QString line = QStringLiteral("%1\t%2\t%3\t%4\t%5\t%6\t%7\n")
                             .arg(frame.timestampUs)
                             .arg(sanitizeTsvField(chunkFileName))
                             .arg(chunkOffset)
                             .arg(frameBytes)
                             .arg(sanitizeTsvField(frame.topic))
                             .arg(sanitizeTsvField(frame.source))
                             .arg(frame.payload.size());
    rtspTimelineFile_->write(line.toUtf8());
    ++rtspTimelineCount_;

    RtspTopicSummary summary = rtspTopicSummary_.value(frame.topic);
    ++summary.frameCount;
    summary.payloadBytes += frame.payload.size();
    if (summary.startUs == 0) summary.startUs = frame.timestampUs;
    summary.endUs = frame.timestampUs;
    rtspTopicSummary_.insert(frame.topic, summary);
}

void RecordReplayController::writeRtspIndexFile(const QString& sessionDir,
                                                qint64 frameCount,
                                                const QMap<QString, RtspTopicSummary>& topicSummary) const {
    if (sessionDir.trimmed().isEmpty() || frameCount <= 0 || topicSummary.isEmpty()) return;

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("rr_rtsp_index_v1"));
    root.insert(QStringLiteral("generatedAtUs"), nowUs());
    root.insert(QStringLiteral("timelineFile"), rtspTimelineFileName());
    root.insert(QStringLiteral("frameCount"), frameCount);

    QJsonArray topics;
    for (auto it = topicSummary.constBegin(); it != topicSummary.constEnd(); ++it) {
        const RtspTopicSummary& summary = it.value();
        QJsonObject node;
        node.insert(QStringLiteral("topic"), it.key());
        node.insert(QStringLiteral("frameCount"), summary.frameCount);
        node.insert(QStringLiteral("payloadBytes"), summary.payloadBytes);
        node.insert(QStringLiteral("startUs"), summary.startUs);
        node.insert(QStringLiteral("endUs"), summary.endUs);
        node.insert(QStringLiteral("startTime"), formatDateTime(summary.startUs));
        node.insert(QStringLiteral("endTime"), formatDateTime(summary.endUs));
        topics.push_back(node);
    }
    root.insert(QStringLiteral("topics"), topics);

    QFile indexFile(QDir(sessionDir).filePath(rtspIndexFileName()));
    if (!indexFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    indexFile.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

void RecordReplayController::resetRtspIndexState() {
    if (rtspTimelineFile_ != nullptr) {
        rtspTimelineFile_->close();
        delete rtspTimelineFile_;
        rtspTimelineFile_ = nullptr;
    }
    rtspTimelineCount_ = 0;
    rtspTopicSummary_.clear();
}

QString RecordReplayController::sanitizeTsvField(QString value) {
    value.replace(QLatin1Char('\t'), QLatin1Char(' '));
    value.replace(QLatin1Char('\r'), QLatin1Char(' '));
    value.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return value;
}

QString RecordReplayController::rtspTimelineFileName() { return QString::fromLatin1(kRtspTimelineFile); }

QString RecordReplayController::rtspIndexFileName() { return QString::fromLatin1(kRtspIndexFile); }

void RecordReplayController::startReplayThread(SessionData session, bool useRange,
                                               qint64 effectiveBeginUs, qint64 effectiveEndUs,
                                               qint64 displayBeginUs, qint64 displayEndUs) {
    stopReplayThread();
    replayStop_.store(false, std::memory_order_release);
    replaySeekRequestUs_.store(-1, std::memory_order_release);
    replaying_.store(true, std::memory_order_release);
    replayStartMs_ = QDateTime::currentMSecsSinceEpoch();
    replayProgress_.store(0.0, std::memory_order_release);
    replayRangeBeginUs_.store(displayBeginUs, std::memory_order_release);
    replayRangeEndUs_.store(displayEndUs, std::memory_order_release);
    replayCurrentUs_.store(displayBeginUs, std::memory_order_release);
    setOperationFeedback(QStringLiteral("重演已开始：%1").arg(session.sessionId));
    replayThread_ = std::thread([this, session = std::move(session), useRange,
                                 effectiveBeginUs, effectiveEndUs,
                                 displayBeginUs, displayEndUs]() {
        replayLoop(session, useRange, effectiveBeginUs, effectiveEndUs, displayBeginUs, displayEndUs);
    });
    invokeRuntimeChanged();
}

void RecordReplayController::stopReplayThread() {
    replayStop_.store(true, std::memory_order_release);
    replaySleepCv_.notify_all();
    if (replayThread_.joinable()) replayThread_.join();
    replayStop_.store(false, std::memory_order_release);
    replaySeekRequestUs_.store(-1, std::memory_order_release);
}

void RecordReplayController::replayLoop(SessionData session, bool useRange,
                                        qint64 effectiveBeginUs, qint64 effectiveEndUs,
                                        qint64 displayBeginUs, qint64 displayEndUs) {
    publishStatus(QStringLiteral("replay_started"), QVariantMap{{QStringLiteral("sessionId"), session.sessionId}});
    qint64 previousUs = 0;
    bool firstFrame = true;
    bool applyHeadPadding = useRange;
    const qint64 totalRangeUs = std::max<qint64>(1, displayEndUs - displayBeginUs);
    qint64 replayCursorUs = effectiveBeginUs;
    qint64 notifyCounter = 0;

    auto applySeek = [&](qint64 seekUs) {
        const qint64 clamped = std::max(effectiveBeginUs, std::min(effectiveEndUs, seekUs));
        replayCursorUs = clamped;
        previousUs = 0;
        firstFrame = true;
        applyHeadPadding = false;
        replayCurrentUs_.store(clamped, std::memory_order_release);
        const double progress = std::max(
            0.0,
            std::min(1.0, static_cast<double>(clamped - displayBeginUs) / static_cast<double>(totalRangeUs)));
        replayProgress_.store(progress, std::memory_order_release);
        invokeRuntimeChanged();
    };

    while (!replayStop_.load(std::memory_order_acquire)) {
        bool seekTriggered = false;

        for (const QString& chunk : session.chunks) {
            if (replayStop_.load(std::memory_order_acquire)) {
                break;
            }

            QFile file(QDir(session.directory).filePath(chunk));
            if (!file.open(QIODevice::ReadOnly)) {
                continue;
            }

            while (!replayStop_.load(std::memory_order_acquire)) {
                const qint64 pendingSeekUs = replaySeekRequestUs_.exchange(-1, std::memory_order_acq_rel);
                if (pendingSeekUs >= 0) {
                    applySeek(pendingSeekUs);
                    seekTriggered = true;
                    break;
                }

                DecodedFrame frame;
                ReadFrameResult readResult = readBinaryFrame(&file, &frame, nullptr);
                if (readResult == ReadFrameResult::Eof) {
                    break;
                }
                if (readResult == ReadFrameResult::Corrupt) {
                    break;
                }

                const qint64 tsUs = frame.timestampUs;
                const QString topic = frame.topic;
                const QByteArray& payload = frame.payload;
                if (topic.trimmed().isEmpty()) {
                    continue;
                }
                if (useRange && (tsUs < effectiveBeginUs || tsUs > effectiveEndUs)) {
                    continue;
                }
                if (tsUs < replayCursorUs) {
                    continue;
                }

                if (!firstFrame) {
                    qint64 deltaUs = tsUs - previousUs;
                    if (deltaUs < 0) {
                        deltaUs = 0;
                    }
                    const double currentSpeed = replaySpeed_.load(std::memory_order_acquire);
                    const qint64 waitUs = static_cast<qint64>(deltaUs / std::max(0.1, currentSpeed));
                    if (waitUs > 0) {
                        std::unique_lock<std::mutex> lock(replaySleepMutex_);
                        replaySleepCv_.wait_for(lock, std::chrono::microseconds(waitUs), [this]() {
                            return replayStop_.load(std::memory_order_acquire)
                                || replaySeekRequestUs_.load(std::memory_order_acquire) >= 0;
                        });
                    }
                } else {
                    if (applyHeadPadding && useRange && tsUs > displayBeginUs) {
                        const qint64 headUs = tsUs - displayBeginUs;
                        const double currentSpeed = replaySpeed_.load(std::memory_order_acquire);
                        const qint64 waitUs = static_cast<qint64>(headUs / std::max(0.1, currentSpeed));
                        if (waitUs > 0) {
                            std::unique_lock<std::mutex> lock(replaySleepMutex_);
                            replaySleepCv_.wait_for(lock, std::chrono::microseconds(waitUs), [this]() {
                                return replayStop_.load(std::memory_order_acquire)
                                    || replaySeekRequestUs_.load(std::memory_order_acquire) >= 0;
                            });
                        }
                    }
                    firstFrame = false;
                    applyHeadPadding = false;
                }

                if (replayStop_.load(std::memory_order_acquire)) {
                    break;
                }

                if (replaySeekRequestUs_.load(std::memory_order_acquire) >= 0) {
                    continue;
                }

                previousUs = tsUs;
                replayCursorUs = tsUs + 1;
                if (messageBus_ != nullptr) {
                    messageBus_->publish(topic, payload);
                }
                ++replayPublishedCount_;
                replayCurrentUs_.store(tsUs, std::memory_order_release);
                const double progress = std::max(
                    0.0,
                    std::min(1.0, static_cast<double>(tsUs - displayBeginUs) / static_cast<double>(totalRangeUs)));
                replayProgress_.store(progress, std::memory_order_release);
                ++notifyCounter;
                if (notifyCounter % 20 == 0) {
                    invokeRuntimeChanged();
                }
            }

            if (seekTriggered || replayStop_.load(std::memory_order_acquire)) {
                break;
            }
        }

        if (replayStop_.load(std::memory_order_acquire)) {
            break;
        }

        if (seekTriggered) {
            continue;
        }

        const qint64 seekAfterScanUs = replaySeekRequestUs_.exchange(-1, std::memory_order_acq_rel);
        if (seekAfterScanUs >= 0) {
            applySeek(seekAfterScanUs);
            continue;
        }

        if (useRange && !firstFrame && previousUs < displayEndUs) {
            const qint64 tailUs = displayEndUs - previousUs;
            if (tailUs > 0) {
                const double currentSpeed = replaySpeed_.load(std::memory_order_acquire);
                const qint64 waitUs = static_cast<qint64>(tailUs / std::max(0.1, currentSpeed));
                if (waitUs > 0) {
                    std::unique_lock<std::mutex> lock(replaySleepMutex_);
                    replaySleepCv_.wait_for(lock, std::chrono::microseconds(waitUs), [this]() {
                        return replayStop_.load(std::memory_order_acquire)
                            || replaySeekRequestUs_.load(std::memory_order_acquire) >= 0;
                    });
                }
            }

            if (replayStop_.load(std::memory_order_acquire)) {
                break;
            }

            const qint64 seekAfterTailUs = replaySeekRequestUs_.exchange(-1, std::memory_order_acq_rel);
            if (seekAfterTailUs >= 0) {
                applySeek(seekAfterTailUs);
                continue;
            }
        }

        break;
    }

    bool interrupted = replayStop_.load(std::memory_order_acquire);
    if (!interrupted) {
        replayCurrentUs_.store(displayEndUs, std::memory_order_release);
        replayProgress_.store(1.0, std::memory_order_release);
    }

    publishStatus(QStringLiteral("replay_stopped"), QVariantMap{
        {QStringLiteral("sessionId"), session.sessionId},
        {QStringLiteral("published"), replayPublishedCount_}
    });

    QMetaObject::invokeMethod(this, [this, interrupted]() {
        if (interrupted) {
            setOperationFeedback(QStringLiteral("重演已停止：已发布 %1 条").arg(replayPublishedCount_));
        } else {
            if (replayPublishedCount_ <= 0) {
                setOperationFeedback(QStringLiteral("重演完成：指定时间段内无可发布数据"));
            } else {
                setOperationFeedback(QStringLiteral("重演完成：已发布 %1 条").arg(replayPublishedCount_));
            }
        }
        replaying_.store(false, std::memory_order_release);
        emit runtimeChanged();
    }, Qt::QueuedConnection);
}



