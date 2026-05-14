#pragma once

#include <plugin_api/imessage_bus.h>

#include <QMutex>
#include <QObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

class QFile;

class RecordReplayController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool recording READ recording NOTIFY runtimeChanged)
    Q_PROPERTY(bool replaying READ replaying NOTIFY runtimeChanged)
    Q_PROPERTY(QString recordingStateText READ recordingStateText NOTIFY runtimeChanged)
    Q_PROPERTY(QString recordingElapsedText READ recordingElapsedText NOTIFY runtimeChanged)
    Q_PROPERTY(QString replayElapsedText READ replayElapsedText NOTIFY runtimeChanged)
    Q_PROPERTY(double replayProgress READ replayProgress NOTIFY runtimeChanged)
    Q_PROPERTY(QString replayProgressText READ replayProgressText NOTIFY runtimeChanged)
    Q_PROPERTY(QString operationFeedback READ operationFeedback NOTIFY runtimeChanged)
    Q_PROPERTY(double replaySpeed READ replaySpeed NOTIFY replaySpeedChanged)
    Q_PROPERTY(QString diskSummary READ diskSummary NOTIFY diskSummaryChanged)
    Q_PROPERTY(bool rtspAvailable READ rtspAvailable NOTIFY rtspAvailableChanged)
    Q_PROPERTY(QString topicFilterText READ topicFilterText WRITE setTopicFilterText NOTIFY topicFilterTextChanged)
    Q_PROPERTY(QString recordTargetDir READ recordTargetDir WRITE setRecordTargetDir NOTIFY recordTargetDirChanged)
    Q_PROPERTY(QString activeRecordFile READ activeRecordFile NOTIFY runtimeChanged)
    Q_PROPERTY(QVariantList sessionList READ sessionList NOTIFY sessionListChanged)
    Q_PROPERTY(int selectedSessionIndex READ selectedSessionIndex WRITE setSelectedSessionIndex NOTIFY selectedSessionIndexChanged)
    Q_PROPERTY(QVariantMap selectedSessionInfo READ selectedSessionInfo NOTIFY selectedSessionInfoChanged)
    Q_PROPERTY(QString replayStartText READ replayStartText WRITE setReplayStartText NOTIFY replayRangeChanged)
    Q_PROPERTY(QString replayEndText READ replayEndText WRITE setReplayEndText NOTIFY replayRangeChanged)
    Q_PROPERTY(QString replayModeText READ replayModeText CONSTANT)

public:
    explicit RecordReplayController(QObject* parent = nullptr);
    ~RecordReplayController() override;

    void setMessageBus(IMessageBus* messageBus);
    void onBusMessage(const QString& topic, const QByteArray& payload, const QString& source);

    bool recording() const;
    bool replaying() const;
    QString recordingStateText() const;
    QString recordingElapsedText() const;
    QString replayElapsedText() const;
    double replayProgress() const;
    QString replayProgressText() const;
    QString operationFeedback() const;
    double replaySpeed() const;
    QString diskSummary() const;
    bool rtspAvailable() const;
    void setRtspAvailable(bool available);
    QString topicFilterText() const;
    void setTopicFilterText(const QString& text);
    QString recordTargetDir() const;
    void setRecordTargetDir(const QString& dirPath);
    QString activeRecordFile() const;
    QVariantList sessionList() const;
    int selectedSessionIndex() const;
    void setSelectedSessionIndex(int index);
    QVariantMap selectedSessionInfo() const;
    QString replayStartText() const;
    void setReplayStartText(const QString& text);
    QString replayEndText() const;
    void setReplayEndText(const QString& text);
    QString replayModeText() const;

    Q_INVOKABLE QString chooseRecordDirectory();
    Q_INVOKABLE void refreshSessionList();
    Q_INVOKABLE bool startRecord();
    Q_INVOKABLE void stopRecord();
    Q_INVOKABLE bool startReplay(bool useRange);
    Q_INVOKABLE void stopReplay();
    Q_INVOKABLE void selectPreviousSession();
    Q_INVOKABLE void selectNextSession();
    Q_INVOKABLE void decreaseReplaySpeed();
    Q_INVOKABLE void increaseReplaySpeed();
    Q_INVOKABLE bool seekReplayByProgress(double progress);
    Q_INVOKABLE bool removeSelectedSession();
    Q_INVOKABLE bool openSelectedSessionDirectory();
    Q_INVOKABLE bool clipSelectedRange();

signals:
    void runtimeChanged();
    void replaySpeedChanged();
    void diskSummaryChanged();
    void rtspAvailableChanged();
    void topicFilterTextChanged();
    void recordTargetDirChanged();
    void sessionListChanged();
    void selectedSessionIndexChanged();
    void selectedSessionInfoChanged();
    void replayRangeChanged();

private:
    struct RecordFrame {
        qint64 timestampUs = 0;
        QString topic;
        QByteArray payload;
        QString source;
    };

    struct SessionData {
        QString sessionId;
        QString directory;
        qint64 recordCount = 0;
        qint64 payloadBytes = 0;
        qint64 startUs = 0;
        qint64 endUs = 0;
        QStringList chunks;
        qint64 rtspFrameCount = 0;
        int rtspTopicCount = 0;
        QString rtspTopicSummaryText;
        QVariantList rtspTopicRows;
    };

    struct RtspTopicSummary {
        qint64 frameCount = 0;
        qint64 payloadBytes = 0;
        qint64 startUs = 0;
        qint64 endUs = 0;
    };

    bool shouldRecordTopic(const QString& topic) const;
    bool parseRangeTime(const QString& text, qint64* timestampUs) const;
    void invokeRuntimeChanged();
    void setOperationFeedback(const QString& text);
    void refreshDiskSummary();
    void publishStatus(const QString& eventName, const QVariantMap& payload);

    QString defaultRecordTargetDir() const;
    QString defaultRecordRootDir() const;

    SessionData loadSession(const QString& sessionDir) const;
    QVariantMap toSessionInfo(const SessionData& session) const;
    void saveActiveSessionMeta() const;
    bool selectSessionByDirectory(const QString& directory);
    static QString formatDateTime(qint64 timestampUs);
    static QString formatDuration(qint64 milliseconds);
    static QString humanReadableBytes(quint64 bytes);

    void startWriterThread();
    void stopWriterThread(bool flushQueue);
    void writerLoop();
    bool openNextChunk();
    void closeCurrentChunk();
    bool writeFrame(const RecordFrame& frame);
    bool openRtspTimelineFile();
    void closeRtspTimelineFile(bool persistBySession);
    void appendRtspTimeline(const RecordFrame& frame, const QString& chunkFileName, qint64 chunkOffset, qint64 frameBytes);
    void writeRtspIndexFile(const QString& sessionDir, qint64 frameCount, const QMap<QString, RtspTopicSummary>& topicSummary) const;
    void resetRtspIndexState();
    static QString sanitizeTsvField(QString value);
    static QString rtspTimelineFileName();
    static QString rtspIndexFileName();

    void startReplayThread(SessionData session, bool useRange,
                           qint64 effectiveBeginUs, qint64 effectiveEndUs,
                           qint64 displayBeginUs, qint64 displayEndUs);
    void stopReplayThread();
    void replayLoop(SessionData session, bool useRange,
                    qint64 effectiveBeginUs, qint64 effectiveEndUs,
                    qint64 displayBeginUs, qint64 displayEndUs);

private:
    static constexpr qint64 kChunkSizeBytes = 256LL * 1024LL * 1024LL;
    static constexpr qint64 kQueueLimitBytes = 512LL * 1024LL * 1024LL;

    IMessageBus* messageBus_ = nullptr;
    QString topicFilterText_ = QStringLiteral("net/;serial/;can/");
    bool rtspAvailable_ = false;
    QString recordTargetDir_;

    std::atomic<bool> recording_{false};
    std::atomic<bool> replaying_{false};
    qint64 recordStartMs_ = 0;
    qint64 replayStartMs_ = 0;
    std::atomic<double> replayProgress_{0.0};
    std::atomic<qint64> replayRangeBeginUs_{0};
    std::atomic<qint64> replayRangeEndUs_{0};
    std::atomic<qint64> replayCurrentUs_{0};
    std::atomic<double> replaySpeed_{1.0};
    QTimer* runtimeTicker_ = nullptr;

    qint64 replayPublishedCount_ = 0;
    qint64 droppedFrameCount_ = 0;

    QString replayStartText_;
    QString replayEndText_;
    QString activeRecordFile_;

    mutable QMutex stateMutex_;
    QVariantList sessionList_;
    int selectedSessionIndex_ = -1;
    QVariantMap selectedSessionInfo_;
    QString diskSummary_;
    QString operationFeedback_ = QStringLiteral("就绪");

    SessionData activeSession_;
    int currentChunkIndex_ = 0;
    qint64 currentChunkBytes_ = 0;
    QFile* currentChunkFile_ = nullptr;
    QFile* rtspTimelineFile_ = nullptr;
    qint64 rtspTimelineCount_ = 0;
    QMap<QString, RtspTopicSummary> rtspTopicSummary_;

    std::thread writerThread_;
    std::atomic<bool> writerStop_{false};
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::deque<RecordFrame> queue_;
    qint64 queuedBytes_ = 0;

    std::thread replayThread_;
    std::atomic<bool> replayStop_{false};
    std::atomic<qint64> replaySeekRequestUs_{-1};
    std::mutex replaySleepMutex_;
    std::condition_variable replaySleepCv_;
};
