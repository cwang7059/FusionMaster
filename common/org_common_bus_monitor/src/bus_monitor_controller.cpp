#include "bus_monitor_controller.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace {

QString defaultBusRecordTarget();

bool isRuntimeBusPlaceholder(const QString& source) {
    const QString value = source.trimmed();
    if (value.isEmpty()) {
        return true;
    }
    const QString lowered = value.toLower();
    return lowered == QStringLiteral("runtime")
        || lowered == QStringLiteral("runtime_bus")
        || lowered == QStringLiteral("message_bus")
        || lowered == QStringLiteral("bus")
        || value == QStringLiteral("运行时")
        || value == QStringLiteral("运行时总线")
        || value == QStringLiteral("消息总线");
}

QString inferSenderFromTopic(const QString& topic) {
    const QString value = topic.trimmed().toLower();
    if (value.startsWith(QStringLiteral("net/"))) {
        return QStringLiteral("org_common_net_receiver");
    }
    if (value.startsWith(QStringLiteral("serial/"))) {
        return QStringLiteral("org_common_serial_receiver");
    }
    if (value.startsWith(QStringLiteral("can/"))) {
        return QStringLiteral("org_common_can_receiver");
    }
    if (value.startsWith(QStringLiteral("rtsp/"))) {
        return QStringLiteral("org_common_rtsp_replay");
    }
    if (value.startsWith(QStringLiteral("ui/"))) {
        return QStringLiteral("window_manager/app");
    }
    if (value.startsWith(QStringLiteral("record_replay/"))) {
        return QStringLiteral("org_common_record_replay");
    }
    return QString();
}

QString inferSenderFromJsonPayload(const QByteArray& payload) {
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return QString();
    }

    const QJsonObject obj = doc.object();
    static const QStringList keys = {
        QStringLiteral("from"),
        QStringLiteral("source"),
        QStringLiteral("sender"),
        QStringLiteral("plugin"),
        QStringLiteral("publisher")
    };

    for (const QString& key : keys) {
        const QString value = obj.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }

    return QString();
}

QString normalizeBusSource(const QString& topic, const QByteArray& payload, const QString& source) {
    const QString directSource = source.trimmed();
    if (!isRuntimeBusPlaceholder(directSource)) {
        return directSource;
    }
    const QString payloadSource = inferSenderFromJsonPayload(payload);
    if (!payloadSource.isEmpty()) {
        return payloadSource;
    }
    const QString topicSource = inferSenderFromTopic(topic);
    if (!topicSource.isEmpty()) {
        return topicSource;
    }
    return QStringLiteral("unknown");
}

QString sanitizeFileName(QString text) {
    text = text.trimmed();
    if (text.isEmpty()) {
        return QStringLiteral("bus");
    }

    static const QRegularExpression invalidPattern(QStringLiteral(R"([\\/:*?"<>|])"));
    text.replace(invalidPattern, QStringLiteral("_"));
    text.replace(QStringLiteral(" "), QStringLiteral("_"));
    while (text.contains(QStringLiteral("__"))) {
        text.replace(QStringLiteral("__"), QStringLiteral("_"));
    }
    return text;
}

QString buildBusRecordFilePath(const QString& target) {
    QString value = target.trimmed();
    if (value.isEmpty()) {
        value = QStringLiteral("message_bus");
    }

    const bool looksLikePath = value.contains('\\') || value.contains('/') || value.contains(':');
    QString dirPath;
    QString fileStem;

    if (looksLikePath) {
        QFileInfo info(value);
        const QString suffix = info.suffix().toLower();
        if (suffix == QStringLiteral("log") || suffix == QStringLiteral("txt")) {
            return QDir::cleanPath(value);
        }
        dirPath = QDir::cleanPath(value);
        fileStem = QStringLiteral("bus_record");
    } else {
        dirPath = QDir(defaultBusRecordTarget()).absolutePath();
        fileStem = sanitizeFileName(value);
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    return dir.filePath(QStringLiteral("%1_%2.log").arg(fileStem, stamp));
}

QString defaultBusRecordTarget() {
    const QString appDir = QCoreApplication::applicationDirPath();
    return QDir::cleanPath(QDir(appDir).filePath(QStringLiteral("record/org_common_bus_monitor")));
}

void setUtf8TextStream(QTextStream& stream) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

}  // namespace

BusMonitorController::BusMonitorController(QObject* parent)
    : QObject(parent) {}

BusMonitorController::~BusMonitorController() {
    stopBusRecord();
}

void BusMonitorController::appendBusMessage(const QString& topic,
                                            const QByteArray& payload,
                                            const QString& source) {
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    const QString normalizedTopic = topic.trimmed().isEmpty() ? QStringLiteral("(unknown)") : topic.trimmed();
    const QString normalizedSource = normalizeBusSource(normalizedTopic, payload, source);

    QVariantMap record;
    record.insert(QStringLiteral("timestamp"), timestamp);
    record.insert(QStringLiteral("timestampMs"), QDateTime::currentMSecsSinceEpoch());
    record.insert(QStringLiteral("topic"), normalizedTopic);
    record.insert(QStringLiteral("size"), payload.size());
    record.insert(
        QStringLiteral("message"),
        QStringLiteral("[%1] size=%2").arg(normalizedTopic).arg(payload.size()));
    record.insert(QStringLiteral("source"), normalizedSource);

    appendBusRecordToFile(record);

    busAllMessages_.push_back(record);
    while (busAllMessages_.size() > kMaxBusMessageCount) {
        busAllMessages_.remove(0);
    }

    if (realtimePaused_) {
        busDirty_ = true;
        return;
    }

    if (!pendingBusRefreshTimer_.isValid()) {
        pendingBusRefreshTimer_.start();
    }

    ++pendingBusRefreshCount_;
    const bool batchReached = pendingBusRefreshCount_ >= 50;
    const bool intervalReached = pendingBusRefreshTimer_.elapsed() >= 100;
    if (batchReached || intervalReached) {
        pendingBusRefreshCount_ = 0;
        pendingBusRefreshTimer_.restart();
        rebuildBusMessages();
    }
}

QVariantList BusMonitorController::busMessages() const {
    return busMessages_;
}

QString BusMonitorController::busTopicPrefix() const {
    return busTopicPrefix_;
}

void BusMonitorController::setBusTopicPrefix(const QString& prefix) {
    const QString nextPrefix = prefix.trimmed();
    if (busTopicPrefix_ == nextPrefix) {
        return;
    }

    busTopicPrefix_ = nextPrefix;
    emit busTopicPrefixChanged();
    rebuildBusMessages();
}

int BusMonitorController::busTotalCount() const {
    return busAllMessages_.size();
}

int BusMonitorController::busFilteredCount() const {
    return busMessages_.size();
}

QString BusMonitorController::busRecordDefaultTarget() const {
    return QDir::toNativeSeparators(defaultBusRecordTarget());
}

bool BusMonitorController::busRecordRunning() const {
    return busRecordRunning_;
}

QString BusMonitorController::busRecordState() const {
    return busRecordRunning_ ? QStringLiteral("录制中") : QStringLiteral("未开始");
}

QString BusMonitorController::busRecordFilePath() const {
    return busRecordFilePath_;
}

bool BusMonitorController::realtimePaused() const {
    return realtimePaused_;
}

void BusMonitorController::clear() {
    busAllMessages_.clear();
    busMessages_.clear();
    busTopicPrefix_.clear();
    busDirty_ = false;
    pendingBusRefreshCount_ = 0;
    pendingBusRefreshTimer_.invalidate();

    emit busTopicPrefixChanged();
    emit busMessagesChanged();
    emit busCountsChanged();
}

QString BusMonitorController::pickRecordDirectory() {
    return QFileDialog::getExistingDirectory(
        nullptr,
        QStringLiteral("选择总线记录目录"),
        busRecordDefaultTarget(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

bool BusMonitorController::startBusRecord(const QString& target) {
    stopBusRecord();

    const QString filePath = buildBusRecordFilePath(target);
    if (filePath.trimmed().isEmpty()) {
        return false;
    }

    busRecordFile_.setFileName(filePath);
    if (!busRecordFile_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        busRecordFilePath_.clear();
        busRecordRunning_ = false;
        emit busRecordStateChanged();
        return false;
    }

    busRecordFilePath_ = QDir::toNativeSeparators(filePath);
    busRecordRunning_ = true;

    QTextStream stream(&busRecordFile_);
    setUtf8TextStream(stream);
    stream << "# bus record start "
           << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
           << '\n';
    stream.flush();

    emit busRecordStateChanged();
    return true;
}

void BusMonitorController::stopBusRecord() {
    if (busRecordRunning_ && busRecordFile_.isOpen()) {
        QTextStream stream(&busRecordFile_);
        setUtf8TextStream(stream);
        stream << "# bus record stop "
               << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
               << '\n';
        stream.flush();
        busRecordFile_.close();
    }

    const bool wasRunning = busRecordRunning_;
    busRecordRunning_ = false;
    if (wasRunning) {
        emit busRecordStateChanged();
    }
}

void BusMonitorController::setRealtimePaused(bool paused) {
    if (realtimePaused_ == paused) {
        return;
    }

    realtimePaused_ = paused;
    emit realtimePausedChanged();

    pendingBusRefreshCount_ = 0;
    pendingBusRefreshTimer_.invalidate();

    if (realtimePaused_) {
        return;
    }

    if (busDirty_) {
        rebuildBusMessages();
        busDirty_ = false;
    }
}

void BusMonitorController::appendBusRecordToFile(const QVariantMap& record) {
    if (!busRecordRunning_ || !busRecordFile_.isOpen()) {
        return;
    }

    const QString timestamp = record.value(QStringLiteral("timestamp")).toString();
    const QString topic = record.value(QStringLiteral("topic")).toString();
    const int size = record.value(QStringLiteral("size")).toInt();
    const QString source = record.value(QStringLiteral("source")).toString();

    QTextStream stream(&busRecordFile_);
    setUtf8TextStream(stream);
    stream << '[' << timestamp << "] "
           << '[' << source << "] "
           << '[' << topic << "] "
           << "size=" << size << '\n';
    stream.flush();
}

void BusMonitorController::rebuildBusMessages() {
    QVariantList nextBusFiltered;
    nextBusFiltered.reserve(busAllMessages_.size());

    const QString prefix = busTopicPrefix_.trimmed();
    for (const QVariantMap& record : busAllMessages_) {
        const QString topic = record.value(QStringLiteral("topic")).toString();
        const bool topicMatched = prefix.isEmpty() || topic.startsWith(prefix, Qt::CaseInsensitive);
        if (topicMatched) {
            nextBusFiltered.push_back(record);
        }
    }

    busMessages_ = nextBusFiltered;
    emit busMessagesChanged();
    emit busCountsChanged();
}
