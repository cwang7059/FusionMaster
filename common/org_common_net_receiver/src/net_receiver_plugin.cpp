#include "net_receiver_plugin.h"

#include <plugin_api/imessage_bus.h>
#include <plugin_api/iplugin_manager.h>
#include <plugin_api/ireplay_runtime_state.h>

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QDebug>

#include <limits>
#include <utility>

namespace {

quint16 resolvePort(const char* envName, quint16 fallbackPort) {
    bool ok = false;
    const int value = qEnvironmentVariableIntValue(envName, &ok);
    if (!ok || value <= 0 || value > 65535) {
        return fallbackPort;
    }
    return static_cast<quint16>(value);
}

bool packetLogEnabled() {
    static const bool enabled = []() {
        const QString value = qEnvironmentVariable("OSGI_NET_PACKET_LOG").trimmed().toLower();
        return value == QStringLiteral("1")
            || value == QStringLiteral("true")
            || value == QStringLiteral("yes")
            || value == QStringLiteral("on");
    }();
    return enabled;
}

bool publishEnabled() {
    static const bool enabled = []() {
        const QString value = qEnvironmentVariable("OSGI_NET_DISABLE_PUBLISH").trimmed().toLower();
        return !(value == QStringLiteral("1")
                 || value == QStringLiteral("true")
                 || value == QStringLiteral("yes")
                 || value == QStringLiteral("on"));
    }();
    return enabled;
}

bool parseReplayActiveEvent(const QByteArray& payload, bool* replayActive) {
    if (replayActive == nullptr) {
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return false;
    }

    const QJsonObject root = document.object();
    const QString event = root.value(QStringLiteral("event")).toString();
    if (event == QStringLiteral("replay_started")) {
        *replayActive = true;
        return true;
    }
    if (event == QStringLiteral("replay_stopped")) {
        *replayActive = false;
        return true;
    }
    return false;
}

}  // namespace

void NetReceiverPlugin::start() {
    qInfo() << "[NET] 插件启动";

    runtimeStarted_ = false;
    udpPort_ = resolvePort("OSGI_NET_RECEIVER_UDP_PORT", 50001);
    tcpPort_ = resolvePort("OSGI_NET_RECEIVER_TCP_PORT", 50002);
    replayActive_.store(false, std::memory_order_release);
    replayStatusSubscriptionId_ = 0;

    auto* manager = pluginManager();
    if (manager != nullptr) {
        if (auto* replayState = manager->getService<IReplayRuntimeState>(QStringLiteral("replay_runtime_state"))) {
            replayActive_.store(replayState->replayActive(), std::memory_order_release);
        }
        if (auto* bus = manager->messageBus()) {
            replayStatusSubscriptionId_ = bus->subscribeWithTopic(
                QStringLiteral("record_replay/status"),
                [this](const QString&, const QByteArray& payload) {
                    bool replayActive = false;
                    if (!parseReplayActiveEvent(payload, &replayActive)) {
                        return;
                    }
                    handleReplayStateChanged(replayActive);
                });
        }
    }

    if (replayActive_.load(std::memory_order_acquire)) {
        qInfo() << "[NET] 当前处于重演状态，暂不启动网络接收";
        return;
    }
    startRuntimeReceivers();
}

void NetReceiverPlugin::stop() {
    if (auto* manager = pluginManager()) {
        if (auto* bus = manager->messageBus()) {
            if (replayStatusSubscriptionId_ != 0) {
                bus->unsubscribe(replayStatusSubscriptionId_);
            }
        }
    }
    replayStatusSubscriptionId_ = 0;
    replayActive_.store(false, std::memory_order_release);
    stopRuntimeReceivers();

    qInfo() << "[NET] 插件停止";
}

QString NetReceiverPlugin::name() const {
    return QStringLiteral("org_common_net_receiver");
}

void NetReceiverPlugin::handleReplayStateChanged(bool replayActive) {
    const bool previous = replayActive_.exchange(replayActive, std::memory_order_acq_rel);
    if (previous == replayActive) {
        return;
    }

    if (replayActive) {
        qInfo() << "[NET] 进入重演状态，停止网络接收";
        stopRuntimeReceivers();
    } else {
        qInfo() << "[NET] 离开重演状态，恢复网络接收";
        startRuntimeReceivers();
    }
}

void NetReceiverPlugin::startRuntimeReceivers() {
    if (runtimeStarted_) {
        return;
    }
    setupUdpReceiver(udpPort_);
    setupTcpReceiver(tcpPort_);
    runtimeStarted_ = (udpSocket_ != nullptr || tcpServer_ != nullptr);
}

void NetReceiverPlugin::stopRuntimeReceivers() {
    if (tcpServer_) {
        tcpServer_->close();
    }

    for (QTcpSocket* client : std::as_const(tcpClients_)) {
        if (client == nullptr) {
            continue;
        }
        client->disconnectFromHost();
        client->deleteLater();
    }
    tcpClients_.clear();

    if (udpSocket_) {
        udpSocket_->close();
    }

    udpSocket_.reset();
    tcpServer_.reset();
    runtimeStarted_ = false;
}

void NetReceiverPlugin::setupUdpReceiver(quint16 port) {
    udpSocket_ = std::make_unique<QUdpSocket>();

    if (!udpSocket_->bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        qWarning() << "[NET][UDP] 绑定失败，端口=" << port << ", 错误=" << udpSocket_->errorString();
        udpSocket_.reset();
        return;
    }

    QObject::connect(udpSocket_.get(), &QUdpSocket::readyRead, [this]() {
        if (!udpSocket_) {
            return;
        }

        while (udpSocket_->hasPendingDatagrams()) {
            const qint64 pendingSize = udpSocket_->pendingDatagramSize();
            if (pendingSize < 0 || pendingSize > std::numeric_limits<int>::max()) {
                qWarning() << "[NET][UDP] invalid datagram size:" << pendingSize;
                break;
            }

            QByteArray datagram;
            datagram.resize(static_cast<int>(pendingSize));

            QHostAddress sender;
            quint16 senderPort = 0;
            udpSocket_->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

            publish(QStringLiteral("net/udp/raw"), datagram);

            if (packetLogEnabled()) {
                qDebug().noquote()
                    << QStringLiteral("[NET][UDP] %1:%2 -> %3 bytes")
                           .arg(sender.toString())
                           .arg(senderPort)
                           .arg(datagram.size());
            }
        }
    });

    qInfo() << "[NET][UDP] 开始监听端口" << port;
}

void NetReceiverPlugin::setupTcpReceiver(quint16 port) {
    tcpServer_ = std::make_unique<QTcpServer>();

    QObject::connect(tcpServer_.get(), &QTcpServer::newConnection, [this]() {
        if (!tcpServer_) {
            return;
        }

        while (tcpServer_->hasPendingConnections()) {
            QTcpSocket* client = tcpServer_->nextPendingConnection();
            if (client == nullptr) {
                continue;
            }

            tcpClients_.push_back(client);
            qInfo() << "[NET][TCP] 客户端接入" << client->peerAddress().toString() << client->peerPort();

            QObject::connect(client, &QTcpSocket::readyRead, [this, client]() {
                if (client == nullptr) {
                    return;
                }

                const QByteArray payload = client->readAll();
                if (payload.isEmpty()) {
                    return;
                }

                publish(QStringLiteral("net/tcp/raw"), payload);

                if (packetLogEnabled()) {
                    qDebug().noquote()
                        << QStringLiteral("[NET][TCP] %1:%2 -> %3 bytes")
                               .arg(client->peerAddress().toString())
                               .arg(client->peerPort())
                               .arg(payload.size());
                }
            });

            QObject::connect(client, &QTcpSocket::disconnected, [this, client]() {
                tcpClients_.removeAll(client);
                qInfo() << "[NET][TCP] 客户端断开";
                client->deleteLater();
            });
        }
    });

    if (!tcpServer_->listen(QHostAddress::AnyIPv4, port)) {
        qWarning() << "[NET][TCP] 监听失败，端口=" << port << ", 错误=" << tcpServer_->errorString();
        tcpServer_.reset();
        return;
    }

    qInfo() << "[NET][TCP] 开始监听端口" << port;
}

void NetReceiverPlugin::publish(const QString& topic, const QByteArray& payload) const {
    if (!publishEnabled()) {
        return;
    }
    if (replayActive_.load(std::memory_order_acquire)) {
        return;
    }

    auto* manager = pluginManager();
    if (manager == nullptr || manager->messageBus() == nullptr) {
        return;
    }

    manager->messageBus()->publish(topic, payload);
}
