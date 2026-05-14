#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QFile>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class BusMonitorController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList busMessages READ busMessages NOTIFY busMessagesChanged)
    Q_PROPERTY(QString busTopicPrefix READ busTopicPrefix WRITE setBusTopicPrefix NOTIFY busTopicPrefixChanged)
    Q_PROPERTY(int busTotalCount READ busTotalCount NOTIFY busCountsChanged)
    Q_PROPERTY(int busFilteredCount READ busFilteredCount NOTIFY busCountsChanged)
    Q_PROPERTY(QString busRecordDefaultTarget READ busRecordDefaultTarget CONSTANT)
    Q_PROPERTY(bool busRecordRunning READ busRecordRunning NOTIFY busRecordStateChanged)
    Q_PROPERTY(QString busRecordState READ busRecordState NOTIFY busRecordStateChanged)
    Q_PROPERTY(QString busRecordFilePath READ busRecordFilePath NOTIFY busRecordStateChanged)
    Q_PROPERTY(bool realtimePaused READ realtimePaused NOTIFY realtimePausedChanged)

public:
    explicit BusMonitorController(QObject* parent = nullptr);
    ~BusMonitorController() override;

    void appendBusMessage(const QString& topic, const QByteArray& payload, const QString& source);

    QVariantList busMessages() const;
    QString busTopicPrefix() const;
    void setBusTopicPrefix(const QString& prefix);
    int busTotalCount() const;
    int busFilteredCount() const;
    QString busRecordDefaultTarget() const;
    bool busRecordRunning() const;
    QString busRecordState() const;
    QString busRecordFilePath() const;
    bool realtimePaused() const;

    Q_INVOKABLE void clear();
    Q_INVOKABLE QString pickRecordDirectory();
    Q_INVOKABLE bool startBusRecord(const QString& target);
    Q_INVOKABLE void stopBusRecord();
    Q_INVOKABLE void setRealtimePaused(bool paused);

signals:
    void busMessagesChanged();
    void busTopicPrefixChanged();
    void busCountsChanged();
    void busRecordStateChanged();
    void realtimePausedChanged();

private:
    void appendBusRecordToFile(const QVariantMap& record);
    void rebuildBusMessages();

private:
    static constexpr int kMaxBusMessageCount = 5000;

    QVector<QVariantMap> busAllMessages_;
    QVariantList busMessages_;
    QString busTopicPrefix_;
    bool busRecordRunning_ = false;
    QString busRecordFilePath_;
    QFile busRecordFile_;
    bool realtimePaused_ = false;
    bool busDirty_ = false;
    int pendingBusRefreshCount_ = 0;
    QElapsedTimer pendingBusRefreshTimer_;
};

