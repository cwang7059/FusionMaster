#pragma once

#include <QByteArray>
#include <QString>

#include <cstdint>
#include <functional>

using SubscriptionId = std::uint64_t;

class IMessageBus {
public:
    virtual ~IMessageBus() = default;

    // data 建议直接传 Protobuf 序列化后的二进制，避免二次封包。
    virtual void publish(const QString& topic, const QByteArray& data) = 0;

    // 主线程回调，适合业务插件直接更新界面。
    virtual SubscriptionId subscribe(const QString& topic, std::function<void(const QByteArray&)> callback) = 0;

    // 后台线程回调，适合对时延敏感的场景。
    virtual SubscriptionId subscribeRaw(const QString& topic, std::function<void(const QByteArray&)> callback) = 0;

    // 订阅所有 topic，回调中可拿到 topic 名称。
    virtual SubscriptionId subscribeAll(
        std::function<void(const QString& topic, const QByteArray& data)> callback) = 0;

    // 按 topic 前缀订阅，并在回调中返回实际 topic。
    // 例如 prefix=net/ 时，只接收 net/ 开头的消息。
    virtual SubscriptionId subscribeWithTopic(
        const QString& topicPrefix,
        std::function<void(const QString& topic, const QByteArray& data)> callback) = 0;

    // 同 subscribeWithTopic，但回调在后台线程执行。
    virtual SubscriptionId subscribeWithTopicRaw(
        const QString& topicPrefix,
        std::function<void(const QString& topic, const QByteArray& data)> callback) = 0;

    virtual void unsubscribe(SubscriptionId id) = 0;
};
