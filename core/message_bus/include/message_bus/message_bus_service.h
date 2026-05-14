#pragma once

#include <plugin_api/imessage_bus.h>

#include <QObject>
#include <QString>
#include <QVariantList>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <QStringList>
#include <vector>

class MessageBusService final : public QObject, public IMessageBus {
    Q_OBJECT

public:
    explicit MessageBusService(QObject* parent = nullptr);
    ~MessageBusService() override;

    void publish(const QString& topic, const QByteArray& data) override;
    Q_INVOKABLE void publishText(const QString& topic, const QString& text);

    SubscriptionId subscribe(const QString& topic, std::function<void(const QByteArray&)> callback) override;
    SubscriptionId subscribeRaw(const QString& topic, std::function<void(const QByteArray&)> callback) override;
    SubscriptionId subscribeAll(
        std::function<void(const QString& topic, const QByteArray& data)> callback) override;
    SubscriptionId subscribeWithTopic(
        const QString& topicPrefix,
        std::function<void(const QString& topic, const QByteArray& data)> callback) override;
    SubscriptionId subscribeWithTopicRaw(
        const QString& topicPrefix,
        std::function<void(const QString& topic, const QByteArray& data)> callback) override;
    SubscriptionId subscribeAllRaw(
        std::function<void(const QString& topic, const QByteArray& data)> callback);

    void unsubscribe(SubscriptionId id) override;

    Q_INVOKABLE qulonglong publishCount() const;
    Q_INVOKABLE qulonglong publishBytes() const;
    Q_INVOKABLE qulonglong dispatchCount() const;
    Q_INVOKABLE qulonglong mainCallbackCount() const;
    Q_INVOKABLE qulonglong rawCallbackCount() const;
    Q_INVOKABLE qulonglong fallbackCount() const;
    Q_INVOKABLE int subscriptionCount() const;
    Q_INVOKABLE QString metricsSummary() const;
    Q_INVOKABLE QVariantList topTopics(int limit = 10) const;
    Q_INVOKABLE QString topTopicsSummary(int limit = 10) const;
    Q_INVOKABLE void resetMetrics();
    Q_INVOKABLE qint64 publishBurst(const QString& topic, int count, int payloadBytes = 64);
    Q_INVOKABLE void pushOwnerContext(const QString& owner);
    Q_INVOKABLE void popOwnerContext();
    Q_INVOKABLE void setSubscriptionOwner(qulonglong id, const QString& owner);
    Q_INVOKABLE QVariantList subscriptionStats() const;

private:
    struct RuntimeState;

    struct SubscriptionEntry {
        struct TopicReceiveMetrics {
            std::uint64_t count = 0;
            std::uint64_t bytes = 0;
            qint64 lastReceiveMs = 0;
        };

        SubscriptionId id = 0;
        QString topicPrefix;
        QString owner;
        bool runOnMainThread = true;
        std::uint64_t receiveCount = 0;
        std::uint64_t receiveBytes = 0;
        qint64 lastReceiveMs = 0;
        std::function<void(const QString& topic, const QByteArray& data)> callback;
        std::unordered_map<std::string, TopicReceiveMetrics> topicMetrics;
    };

    struct Metrics {
        std::atomic<std::uint64_t> publishCount{0};
        std::atomic<std::uint64_t> publishBytes{0};
        std::atomic<std::uint64_t> dispatchCount{0};
        std::atomic<std::uint64_t> mainCallbackCount{0};
        std::atomic<std::uint64_t> rawCallbackCount{0};
        std::atomic<std::uint64_t> fallbackCount{0};
    };

    struct TopicMetrics {
        std::uint64_t publishCount = 0;
        std::uint64_t publishBytes = 0;
        std::uint64_t dispatchCount = 0;
    };

    SubscriptionId addSubscription(
        const QString& topic,
        bool runOnMainThread,
        std::function<void(const QString& topic, const QByteArray& data)> callback);
    static QString currentOwnerContext();
    static QString sanitizeOwner(const QString& owner);

    void startRuntime();
    void stopRuntime();
    void proxyLoop();
    void dispatchLoop();

    void dispatchMessage(const QString& topic, const QByteArray& data);
    static bool topicMatches(const QString& topic, const QString& prefix);
    static bool parseVerboseFlag();

    void updateTopicPublishMetrics(const QString& topic, int bytes);
    void updateTopicDispatchMetrics(const QString& topic);

    void logRuntimeSummary(const QString& phase) const;

private:
    std::atomic_bool running_{false};
    std::atomic<SubscriptionId> nextSubscriptionId_{1};

    mutable std::mutex subscriptionMutex_;
    std::unordered_map<SubscriptionId, SubscriptionEntry> subscriptions_;

    std::mutex publisherMutex_;

    std::thread proxyThread_;
    std::thread dispatchThread_;

    std::unique_ptr<RuntimeState> runtime_;
    bool verboseLogging_ = false;
    bool useZmqTransport_ = false;
    Metrics metrics_;

    mutable std::mutex topicMetricsMutex_;
    std::unordered_map<std::string, TopicMetrics> topicMetrics_;
};
