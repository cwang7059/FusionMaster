#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/imessage_bus.h>

#include <QList>
#include <QTcpServer>
#include <QUdpSocket>

#include <atomic>
#include <memory>

class QTcpSocket;

class NetReceiverPlugin : public IPlugin {
public:
    void start() override;
    void stop() override;
    QString name() const override;

private:
    void handleReplayStateChanged(bool replayActive);
    void startRuntimeReceivers();
    void stopRuntimeReceivers();
    void setupUdpReceiver(quint16 port);
    void setupTcpReceiver(quint16 port);
    void publish(const QString& topic, const QByteArray& payload) const;

private:
    std::unique_ptr<QUdpSocket> udpSocket_;
    std::unique_ptr<QTcpServer> tcpServer_;
    QList<QTcpSocket*> tcpClients_;
    quint16 udpPort_ = 50001;
    quint16 tcpPort_ = 50002;
    bool runtimeStarted_ = false;
    SubscriptionId replayStatusSubscriptionId_ = 0;
    std::atomic<bool> replayActive_{false};
};
