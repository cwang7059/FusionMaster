#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/imessage_bus.h>

#include <QByteArray>
#include <QObject>
#include <QSet>
#include <QString>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

class IMessageBus;

class RtspReplayPlugin : public IPlugin {
public:
    void start() override;
    void stop() override;
    QString name() const override;

private:
    struct StreamConfig {
        QString streamId;
        QString url;
        QString topic;
    };

    struct StreamWorker {
        StreamConfig config;
        std::atomic<bool> stop{false};
        std::thread thread;
    };

    void startWorkers();
    void stopWorkers();
    void restartWorkers();
    std::vector<StreamConfig> loadConfigs() const;
    std::vector<StreamConfig> loadEnvConfigs() const;
    void runStreamWorker(StreamWorker* worker);
    void handleControlCommand(const QByteArray& payload);
    bool addDynamicStream(
        const QString& requestedStreamId,
        const QString& url,
        QString* effectiveStreamId,
        QString* message,
        bool* configChanged);
    bool removeStream(
        const QString& streamId,
        QString* effectiveStreamId,
        QString* message,
        bool* configChanged);
    void publishControlResult(
        const QString& action,
        const QString& requestId,
        bool success,
        const QString& message,
        bool configChanged,
        const QString& effectiveStreamId) const;

    IMessageBus* messageBus_ = nullptr;
    QObject* controllerMarker_ = nullptr;
    std::vector<std::unique_ptr<StreamWorker>> workers_;
    SubscriptionId replayStatusSubscriptionId_ = 0;
    SubscriptionId controlSubscriptionId_ = 0;
    std::atomic<bool> replayActive_{false};
    std::mutex workersMutex_;
    mutable std::mutex dynamicConfigsMutex_;
    std::vector<StreamConfig> dynamicConfigs_;
    QSet<QString> removedStreamIds_;
};
