#pragma once

#include <plugin_api/imessage_bus.h>

#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QVariantList>

#include <atomic>
#include <memory>

class IPluginManager;
class QTimer;

struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwsContext;

class RtspPlayerController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList streams READ streams NOTIFY streamsChanged)
    Q_PROPERTY(QString selectedStreamId READ selectedStreamId WRITE setSelectedStreamId NOTIFY selectedStreamIdChanged)
    Q_PROPERTY(bool replayActive READ replayActive NOTIFY playbackModeChanged)
    Q_PROPERTY(bool preferPacketIntegrity READ preferPacketIntegrity WRITE setPreferPacketIntegrity NOTIFY packetHandlingModeChanged)
    Q_PROPERTY(QString packetHandlingModeText READ packetHandlingModeText NOTIFY packetHandlingModeChanged)
    Q_PROPERTY(QString playbackModeText READ playbackModeText NOTIFY playbackModeChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString addStreamStatusText READ addStreamStatusText NOTIFY addStreamStatusTextChanged)
    Q_PROPERTY(QString selectedTopic READ selectedTopic NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(QString selectedCodecText READ selectedCodecText NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(QString selectedResolutionText READ selectedResolutionText NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(qint64 receivedPacketCount READ receivedPacketCount NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(qint64 decodedFrameCount READ decodedFrameCount NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(int decodedFps READ decodedFps NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(bool waitingForKeyFrame READ waitingForKeyFrame NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(QString lastFrameTimeText READ lastFrameTimeText NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(int selectedQueuedPacketCount READ selectedQueuedPacketCount NOTIFY selectedStreamInfoChanged)
    Q_PROPERTY(qint64 selectedDroppedPacketCount READ selectedDroppedPacketCount NOTIFY selectedStreamInfoChanged)

public:
    explicit RtspPlayerController(QObject* parent = nullptr);
    ~RtspPlayerController() override;

    void setPluginManager(IPluginManager* pluginManager);
    void start();
    void stop();

    QVariantList streams() const;
    QString selectedStreamId() const;
    void setSelectedStreamId(const QString& streamId);

    bool replayActive() const;
    bool preferPacketIntegrity() const;
    void setPreferPacketIntegrity(bool value);
    QString packetHandlingModeText() const;
    QString playbackModeText() const;
    QString statusText() const;
    QString addStreamStatusText() const;
    QString selectedTopic() const;
    QString selectedCodecText() const;
    QString selectedResolutionText() const;
    qint64 receivedPacketCount() const;
    qint64 decodedFrameCount() const;
    int decodedFps() const;
    bool waitingForKeyFrame() const;
    QString lastFrameTimeText() const;
    int selectedQueuedPacketCount() const;
    qint64 selectedDroppedPacketCount() const;
    Q_INVOKABLE void requestAddStream(const QString& url, const QString& streamId);
    Q_INVOKABLE void requestRemoveStream(const QString& streamId);
    Q_INVOKABLE void requestRemoveSelectedStream();
    Q_INVOKABLE void togglePacketHandlingMode();

signals:
    void streamsChanged();
    void selectedStreamIdChanged();
    void playbackModeChanged();
    void packetHandlingModeChanged();
    void statusTextChanged();
    void addStreamStatusTextChanged();
    void selectedStreamInfoChanged();
    void framePresented(const QImage& image, const QString& streamId);
    void frameCleared();

private:
    struct EncodedPacket;
    struct StreamState;
    struct QueueBudget {
        int maxPackets = 0;
        qint64 maxBytes = 0;
    };

    void handleRtspPacket(const QString& topic, const QByteArray& payload);
    void handleReplayStateChanged(bool replayActive);
    void handleControlResult(const QByteArray& payload);
    void publishControlRequest(const QJsonObject& request);
    void initializeConfiguredPullUrls();
    void resetAllStreamStates();
    void removeStreamState(const QString& streamId);
    QString pullUrlForStream(const QString& streamId) const;
    std::shared_ptr<StreamState> streamState(const QString& streamId) const;
    std::shared_ptr<StreamState> ensureStreamState(const QString& streamId, const QString& topic);
    void stopAllStreams();
    void streamWorkerLoop(const std::shared_ptr<StreamState>& stream);
    bool ensureDecoder(StreamState* stream, const EncodedPacket& packet);
    void resetDecoder(StreamState* stream);
    void destroyDecoder(StreamState* stream);
    QImage decodePacket(StreamState* stream, const EncodedPacket& packet, bool* producedFrame);
    void queueFramePresentation(const QString& streamId, const QImage& image);
    void deliverPendingFrame();
    void refreshViewState();
    void updatePresentationTimerInterval();
    QueueBudget queueBudgetForStream(const QString& streamId) const;
    bool shouldDecodeStream(const QString& streamId) const;
    void trimQueueToBudget(StreamState* stream);
    int swsFlagsForCurrentMode() const;

    static bool parseReplayActiveEvent(const QByteArray& payload, bool* replayActive);
    static QString codecName(int codecId);
    static QString formatWallClock(qint64 wallClockUs);

    IPluginManager* pluginManager_ = nullptr;
    IMessageBus* messageBus_ = nullptr;
    SubscriptionId rtspSubscriptionId_ = 0;
    SubscriptionId replayStatusSubscriptionId_ = 0;
    SubscriptionId controlResultSubscriptionId_ = 0;
    std::atomic<bool> replayActive_{false};
    std::atomic<bool> preferPacketIntegrity_{true};
    QTimer* uiRefreshTimer_ = nullptr;
    QTimer* framePresentTimer_ = nullptr;
    QString addStreamStatusText_;

    mutable QMutex streamsMutex_;
    QHash<QString, std::shared_ptr<StreamState>> streams_;
    QHash<QString, QString> streamPullUrls_;
    QHash<QString, QString> pendingRequestUrls_;
    QString selectedStreamId_;
    QString preferredStreamId_;

    mutable QMutex frameMutex_;
    QString pendingFrameStreamId_;
    QImage pendingFrame_;
    bool frameDispatchPending_ = false;
    int framePresentationIntervalMs_ = 33;
};
