#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class LoggingService;

class LogViewerController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList tags READ tags NOTIFY tagsChanged)
    Q_PROPERTY(QStringList levels READ levels CONSTANT)
    Q_PROPERTY(QVariantList filteredLogs READ filteredLogs NOTIFY filteredLogsChanged)
    Q_PROPERTY(QString activeTag READ activeTag WRITE setActiveTag NOTIFY activeTagChanged)
    Q_PROPERTY(QString activeLevel READ activeLevel WRITE setActiveLevel NOTIFY activeLevelChanged)
    Q_PROPERTY(QString keyword READ keyword WRITE setKeyword NOTIFY keywordChanged)
    Q_PROPERTY(QString importStatus READ importStatus NOTIFY importStatusChanged)
    Q_PROPERTY(QString logRootDir READ logRootDir NOTIFY logRootDirChanged)
    Q_PROPERTY(int totalLogCount READ totalLogCount NOTIFY countsChanged)
    Q_PROPERTY(int filteredLogCount READ filteredLogCount NOTIFY countsChanged)
    Q_PROPERTY(bool realtimePaused READ realtimePaused NOTIFY realtimePausedChanged)

public:
    explicit LogViewerController(QObject* parent = nullptr);

    void bindLoggingService(LoggingService* loggingService);

    QStringList tags() const;
    QStringList levels() const;
    QVariantList filteredLogs() const;
    QString activeTag() const;
    void setActiveTag(const QString& tag);
    QString activeLevel() const;
    void setActiveLevel(const QString& level);
    QString keyword() const;
    void setKeyword(const QString& keyword);
    QString importStatus() const;
    QString logRootDir() const;
    int totalLogCount() const;
    int filteredLogCount() const;
    bool realtimePaused() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void importLogFile(const QString& filePath);
    Q_INVOKABLE void importLogDirectory(const QString& dirPath);
    Q_INVOKABLE QString pickAndImportLogFile();
    Q_INVOKABLE QString pickAndImportLogDirectory();
    Q_INVOKABLE void setRealtimePaused(bool paused);

signals:
    void tagsChanged();
    void filteredLogsChanged();
    void activeTagChanged();
    void activeLevelChanged();
    void keywordChanged();
    void importStatusChanged();
    void logRootDirChanged();
    void countsChanged();
    void realtimePausedChanged();

private slots:
    void onRuntimeLogRecord(const QVariantMap& record);

private:
    void appendRecord(QVariantMap record);
    void rebuildTags();
    void rebuildFilteredLogs();
    QVariantMap parseLogLine(const QString& line) const;
    void setImportStatus(const QString& status);

private:
    static constexpr int kMaxLogCount = 10000;

    QVector<QVariantMap> allLogs_;
    QStringList tags_;
    QStringList levels_;
    QVariantList filteredLogs_;
    QString activeTag_;
    QString activeLevel_;
    QString keyword_;
    QString importStatus_;
    QString logRootDir_;
    bool realtimePaused_ = false;
    bool logsDirty_ = false;
};

