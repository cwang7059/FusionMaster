#include "log_viewer_controller.h"

#include <logging/logging_service.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace {

const QString kAllTag = QStringLiteral("全部");
const QString kAllLevel = QStringLiteral("全部");

void setUtf8TextStream(QTextStream& stream) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

QString normalizeLevel(QString level) {
    level = level.trimmed().toUpper();
    if (level == QStringLiteral("DEBUG")
        || level == QStringLiteral("INFO")
        || level == QStringLiteral("WARN")
        || level == QStringLiteral("ERROR")
        || level == QStringLiteral("CRITICAL")) {
        return level;
    }
    return QStringLiteral("INFO");
}

bool matchesKeyword(const QVariantMap& record, const QString& keyword) {
    if (keyword.trimmed().isEmpty()) {
        return true;
    }

    const QString message = record.value(QStringLiteral("message")).toString();
    return message.contains(keyword, Qt::CaseInsensitive);
}

}  // namespace

LogViewerController::LogViewerController(QObject* parent)
    : QObject(parent)
    , tags_{kAllTag}
    , levels_{kAllLevel,
              QStringLiteral("DEBUG"),
              QStringLiteral("INFO"),
              QStringLiteral("WARN"),
              QStringLiteral("ERROR"),
              QStringLiteral("CRITICAL")}
    , activeTag_(kAllTag)
    , activeLevel_(kAllLevel)
    , importStatus_(QStringLiteral("未导入日志"))
    , logRootDir_(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("logs"))) {}

void LogViewerController::bindLoggingService(LoggingService* loggingService) {
    if (loggingService == nullptr) {
        return;
    }

    connect(loggingService,
            &LoggingService::logRecordReady,
            this,
            &LogViewerController::onRuntimeLogRecord,
            Qt::QueuedConnection);

    const QString rootDir = loggingService->logRootDir();
    if (!rootDir.isEmpty() && rootDir != logRootDir_) {
        logRootDir_ = rootDir;
        emit logRootDirChanged();
    }
}

QStringList LogViewerController::tags() const {
    return tags_;
}

QStringList LogViewerController::levels() const {
    return levels_;
}

QVariantList LogViewerController::filteredLogs() const {
    return filteredLogs_;
}

QString LogViewerController::activeTag() const {
    return activeTag_;
}

void LogViewerController::setActiveTag(const QString& tag) {
    QString nextTag = tag.trimmed();
    if (nextTag.isEmpty() || !tags_.contains(nextTag)) {
        nextTag = kAllTag;
    }

    if (activeTag_ == nextTag) {
        return;
    }

    activeTag_ = nextTag;
    emit activeTagChanged();
    rebuildFilteredLogs();
}

QString LogViewerController::activeLevel() const {
    return activeLevel_;
}

void LogViewerController::setActiveLevel(const QString& level) {
    QString nextLevel = normalizeLevel(level);
    if (level.trimmed().isEmpty() || level == kAllLevel || !levels_.contains(nextLevel)) {
        nextLevel = kAllLevel;
    }

    if (activeLevel_ == nextLevel) {
        return;
    }

    activeLevel_ = nextLevel;
    emit activeLevelChanged();
    rebuildFilteredLogs();
}

QString LogViewerController::keyword() const {
    return keyword_;
}

void LogViewerController::setKeyword(const QString& keyword) {
    const QString nextKeyword = keyword.trimmed();
    if (keyword_ == nextKeyword) {
        return;
    }

    keyword_ = nextKeyword;
    emit keywordChanged();
    rebuildFilteredLogs();
}

QString LogViewerController::importStatus() const {
    return importStatus_;
}

QString LogViewerController::logRootDir() const {
    return logRootDir_;
}

int LogViewerController::totalLogCount() const {
    return allLogs_.size();
}

int LogViewerController::filteredLogCount() const {
    return filteredLogs_.size();
}

bool LogViewerController::realtimePaused() const {
    return realtimePaused_;
}

void LogViewerController::clear() {
    allLogs_.clear();
    logsDirty_ = false;

    if (activeTag_ != kAllTag) {
        activeTag_ = kAllTag;
        emit activeTagChanged();
    }

    if (activeLevel_ != kAllLevel) {
        activeLevel_ = kAllLevel;
        emit activeLevelChanged();
    }

    if (!keyword_.isEmpty()) {
        keyword_.clear();
        emit keywordChanged();
    }

    if (tags_ != QStringList{kAllTag}) {
        tags_ = QStringList{kAllTag};
        emit tagsChanged();
    }

    if (!filteredLogs_.isEmpty()) {
        filteredLogs_.clear();
        emit filteredLogsChanged();
    }

    emit countsChanged();
    setImportStatus(QStringLiteral("已清空日志列表"));
}

void LogViewerController::importLogFile(const QString& filePath) {
    const QString path = QDir::cleanPath(filePath.trimmed());
    if (path.isEmpty()) {
        setImportStatus(QStringLiteral("导入失败：文件路径为空"));
        return;
    }

    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        setImportStatus(QStringLiteral("导入失败：文件不存在 %1").arg(path));
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setImportStatus(QStringLiteral("导入失败：无法打开文件 %1").arg(path));
        return;
    }

    QTextStream stream(&file);
    setUtf8TextStream(stream);

    int importedCount = 0;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        const QVariantMap record = parseLogLine(line);
        allLogs_.push_back(record);
        ++importedCount;
    }

    while (allLogs_.size() > kMaxLogCount) {
        allLogs_.remove(0);
    }

    rebuildTags();
    rebuildFilteredLogs();
    setImportStatus(QStringLiteral("已导入 %1 条日志：%2").arg(importedCount).arg(fileInfo.fileName()));
}

void LogViewerController::importLogDirectory(const QString& dirPath) {
    QString path = QDir::cleanPath(dirPath.trimmed());
    if (path.isEmpty()) {
        path = QDir::cleanPath(logRootDir_);
    }

    QDir dir(path);
    if (!dir.exists()) {
        setImportStatus(QStringLiteral("导入失败：目录不存在 %1").arg(path));
        return;
    }

    int fileCount = 0;
    int lineCount = 0;

    QDirIterator iterator(path,
                          QStringList() << QStringLiteral("*.log") << QStringLiteral("*.txt"),
                          QDir::Files,
                          QDirIterator::Subdirectories);

    while (iterator.hasNext()) {
        const QString oneFilePath = iterator.next();
        QFile file(oneFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        ++fileCount;
        QTextStream stream(&file);
        setUtf8TextStream(stream);

        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.isEmpty()) {
                continue;
            }
            allLogs_.push_back(parseLogLine(line));
            ++lineCount;
        }
    }

    while (allLogs_.size() > kMaxLogCount) {
        allLogs_.remove(0);
    }

    rebuildTags();
    rebuildFilteredLogs();
    setImportStatus(QStringLiteral("目录导入完成：文件 %1 个，日志 %2 条").arg(fileCount).arg(lineCount));
}

QString LogViewerController::pickAndImportLogFile() {
    const QString selectedFile = QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("选择日志文件"),
        logRootDir_,
        QStringLiteral("日志文件 (*.log *.txt);;所有文件 (*.*)"));

    if (selectedFile.trimmed().isEmpty()) {
        return QString();
    }

    importLogFile(selectedFile);
    return selectedFile;
}

QString LogViewerController::pickAndImportLogDirectory() {
    const QString selectedDir = QFileDialog::getExistingDirectory(
        nullptr,
        QStringLiteral("选择日志目录"),
        logRootDir_,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (selectedDir.trimmed().isEmpty()) {
        return QString();
    }

    importLogDirectory(selectedDir);
    return selectedDir;
}

void LogViewerController::setRealtimePaused(bool paused) {
    if (realtimePaused_ == paused) {
        return;
    }

    realtimePaused_ = paused;
    emit realtimePausedChanged();

    if (!realtimePaused_ && logsDirty_) {
        rebuildTags();
        rebuildFilteredLogs();
        logsDirty_ = false;
    }
}

void LogViewerController::onRuntimeLogRecord(const QVariantMap& record) {
    appendRecord(record);
}

void LogViewerController::appendRecord(QVariantMap record) {
    if (record.value(QStringLiteral("timestamp")).toString().trimmed().isEmpty()) {
        record.insert(
            QStringLiteral("timestamp"),
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
    }

    record.insert(
        QStringLiteral("level"),
        normalizeLevel(record.value(QStringLiteral("level")).toString()));

    if (record.value(QStringLiteral("tag")).toString().trimmed().isEmpty()) {
        record.insert(QStringLiteral("tag"), QStringLiteral("APP"));
    }

    if (record.value(QStringLiteral("message")).toString().trimmed().isEmpty()) {
        record.insert(QStringLiteral("message"), QStringLiteral("(空消息)"));
    }

    if (record.value(QStringLiteral("source")).toString().trimmed().isEmpty()) {
        record.insert(QStringLiteral("source"), QStringLiteral("运行时"));
    }

    allLogs_.push_back(record);
    while (allLogs_.size() > kMaxLogCount) {
        allLogs_.remove(0);
    }

    if (realtimePaused_) {
        logsDirty_ = true;
        return;
    }

    rebuildTags();
    rebuildFilteredLogs();
}

void LogViewerController::rebuildTags() {
    QStringList nextTags;
    nextTags.push_back(kAllTag);

    QSet<QString> seen;
    for (const QVariantMap& record : allLogs_) {
        const QString tag = record.value(QStringLiteral("tag")).toString().trimmed();
        if (tag.isEmpty() || seen.contains(tag)) {
            continue;
        }
        seen.insert(tag);
        nextTags.push_back(tag);
    }

    const bool tagChanged = nextTags != tags_;
    tags_ = nextTags;

    if (!tags_.contains(activeTag_)) {
        activeTag_ = kAllTag;
        emit activeTagChanged();
    }

    if (tagChanged) {
        emit tagsChanged();
    }
}

void LogViewerController::rebuildFilteredLogs() {
    QVariantList nextLogs;

    for (const QVariantMap& record : allLogs_) {
        const QString tag = record.value(QStringLiteral("tag")).toString();
        const QString level = normalizeLevel(record.value(QStringLiteral("level")).toString());
        const bool tagMatched = (activeTag_ == kAllTag || activeTag_ == tag);
        const bool levelMatched = (activeLevel_ == kAllLevel || activeLevel_ == level);
        const bool keywordMatched = matchesKeyword(record, keyword_);
        if (tagMatched && levelMatched && keywordMatched) {
            nextLogs.push_back(record);
        }
    }

    filteredLogs_ = nextLogs;
    emit filteredLogsChanged();
    emit countsChanged();
}

QVariantMap LogViewerController::parseLogLine(const QString& line) const {
    static const QRegularExpression pattern(
        QStringLiteral("^\\[([^\\]]+)\\]\\s+\\[([^\\]]+)\\]\\s+\\[([^\\]]+)\\]\\s*(.*)$"));

    QVariantMap record;
    const QRegularExpressionMatch match = pattern.match(line.trimmed());
    if (match.hasMatch()) {
        record.insert(QStringLiteral("timestamp"), match.captured(1).trimmed());
        record.insert(QStringLiteral("level"), normalizeLevel(match.captured(2)));
        record.insert(QStringLiteral("tag"), match.captured(3).trimmed());

        const QString body = match.captured(4).trimmed();
        record.insert(QStringLiteral("message"), body.isEmpty() ? QStringLiteral("(空消息)") : body);
    } else {
        record.insert(
            QStringLiteral("timestamp"),
            QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")));
        record.insert(QStringLiteral("level"), QStringLiteral("INFO"));
        record.insert(QStringLiteral("tag"), QStringLiteral("导入"));
        record.insert(QStringLiteral("message"), line.trimmed());
    }

    record.insert(QStringLiteral("source"), QStringLiteral("导入"));
    return record;
}

void LogViewerController::setImportStatus(const QString& status) {
    if (importStatus_ == status) {
        return;
    }
    importStatus_ = status;
    emit importStatusChanged();
}
