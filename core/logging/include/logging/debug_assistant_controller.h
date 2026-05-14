#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class LoggingService;

class DebugAssistantController final : public QObject {
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
    Q_PROPERTY(QVariantList busMessages READ busMessages NOTIFY busMessagesChanged)
    Q_PROPERTY(QString busTopicPrefix READ busTopicPrefix WRITE setBusTopicPrefix NOTIFY busTopicPrefixChanged)
    Q_PROPERTY(int busTotalCount READ busTotalCount NOTIFY busCountsChanged)
    Q_PROPERTY(int busFilteredCount READ busFilteredCount NOTIFY busCountsChanged)
    Q_PROPERTY(QVariantList uiActions READ uiActions NOTIFY uiActionsChanged)
    Q_PROPERTY(QString uiActionIdPrefix READ uiActionIdPrefix WRITE setUiActionIdPrefix NOTIFY uiActionIdPrefixChanged)
    Q_PROPERTY(QString uiActionCommand READ uiActionCommand WRITE setUiActionCommand NOTIFY uiActionCommandChanged)
    Q_PROPERTY(int uiActionTotalCount READ uiActionTotalCount NOTIFY uiActionCountsChanged)
    Q_PROPERTY(int uiActionFilteredCount READ uiActionFilteredCount NOTIFY uiActionCountsChanged)
    Q_PROPERTY(QString busRecordDefaultTarget READ busRecordDefaultTarget CONSTANT)
    Q_PROPERTY(bool busRecordRunning READ busRecordRunning NOTIFY busRecordStateChanged)
    Q_PROPERTY(QString busRecordState READ busRecordState NOTIFY busRecordStateChanged)
    Q_PROPERTY(QString busRecordFilePath READ busRecordFilePath NOTIFY busRecordStateChanged)
    Q_PROPERTY(bool realtimePaused READ realtimePaused NOTIFY realtimePausedChanged)

public:
    explicit DebugAssistantController(QObject* parent = nullptr);

    void bindLoggingService(LoggingService* loggingService);
    void appendBusMessage(const QString& topic, const QByteArray& payload, const QString& source);

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
    QVariantList busMessages() const;
    QString busTopicPrefix() const;
    void setBusTopicPrefix(const QString& prefix);
    int busTotalCount() const;
    int busFilteredCount() const;
    QVariantList uiActions() const;
    QString uiActionIdPrefix() const;
    void setUiActionIdPrefix(const QString& value);
    QString uiActionCommand() const;
    void setUiActionCommand(const QString& value);
    int uiActionTotalCount() const;
    int uiActionFilteredCount() const;
    QString busRecordDefaultTarget() const;
    bool busRecordRunning() const;
    QString busRecordState() const;
    QString busRecordFilePath() const;
    bool realtimePaused() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void importLogFile(const QString& filePath);
    Q_INVOKABLE void importLogDirectory(const QString& dirPath);
    Q_INVOKABLE QString pickAndImportLogFile();
    Q_INVOKABLE QString pickAndImportLogDirectory();
    Q_INVOKABLE QString pickRecordDirectory();
    Q_INVOKABLE bool startBusRecord(const QString& target);
    Q_INVOKABLE void stopBusRecord();
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
    void busMessagesChanged();
    void busTopicPrefixChanged();
    void busCountsChanged();
    void uiActionsChanged();
    void uiActionIdPrefixChanged();
    void uiActionCommandChanged();
    void uiActionCountsChanged();
    void busRecordStateChanged();
    void realtimePausedChanged();

private slots:
    void onRuntimeLogRecord(const QVariantMap& record);

private:
    void appendRecord(QVariantMap record);
    void appendBusRecord(const QVariantMap& record, bool emitSignal);
    void appendBusRecordToFile(const QVariantMap& record);
    void appendUiActionRecord(const QVariantMap& record, bool emitSignal);
    void rebuildTags();
    void rebuildFilteredLogs();
    void rebuildBusMessages();
    void rebuildUiActions();
    QVariantMap parseLogLine(const QString& line) const;
    void setImportStatus(const QString& status);

private:
    static constexpr int kMaxLogCount = 10000;
    static constexpr int kMaxBusMessageCount = 5000;

    QVector<QVariantMap> allLogs_;
    QStringList tags_;
    QStringList levels_;
    QVariantList filteredLogs_;
    QString activeTag_;
    QString activeLevel_;
    QString keyword_;
    QString importStatus_;
    QString logRootDir_;

    QVector<QVariantMap> busAllMessages_;
    QVariantList busMessages_;
    QString busTopicPrefix_;

    QVector<QVariantMap> uiActionAll_;
    QVariantList uiActions_;
    QString uiActionIdPrefix_;
    QString uiActionCommand_;

    bool busRecordRunning_ = false;
    QString busRecordFilePath_;
    QFile busRecordFile_;

    bool realtimePaused_ = false;
    bool logsDirty_ = false;
    bool busDirty_ = false;
    bool uiActionsDirty_ = false;
    int pendingBusRefreshCount_ = 0;
    QElapsedTimer pendingBusRefreshTimer_;
};
