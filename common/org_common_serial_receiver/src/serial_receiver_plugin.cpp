#include "serial_receiver_plugin.h"

#include <plugin_api/imessage_bus.h>
#include <plugin_api/iplugin_manager.h>
#include <plugin_api/ireplay_runtime_state.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QSerialPort>
#include <QDebug>

namespace {

qint32 resolveBaudRate() {
    bool ok = false;
    const int value = qEnvironmentVariableIntValue("OSGI_SERIAL_BAUD", &ok);
    if (!ok || value <= 0) {
        return 115200;
    }
    return static_cast<qint32>(value);
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

void SerialReceiverPlugin::start() {
    qInfo() << "[SERIAL] 插件启动";

    runtimeOpened_ = false;
    portName_ = qEnvironmentVariable("OSGI_SERIAL_PORT").trimmed();
    baudRate_ = resolveBaudRate();
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

    if (portName_.isEmpty()) {
        qInfo() << "[SERIAL] 未设置 OSGI_SERIAL_PORT，串口插件保持空闲";
        return;
    }

    if (replayActive_.load(std::memory_order_acquire)) {
        qInfo() << "[SERIAL] 当前处于重演状态，暂不打开串口";
        return;
    }
    openSerialPort();
}

void SerialReceiverPlugin::stop() {
    if (auto* manager = pluginManager()) {
        if (auto* bus = manager->messageBus()) {
            if (replayStatusSubscriptionId_ != 0) {
                bus->unsubscribe(replayStatusSubscriptionId_);
            }
        }
    }
    replayStatusSubscriptionId_ = 0;
    replayActive_.store(false, std::memory_order_release);
    closeSerialPort();

    qInfo() << "[SERIAL] 插件停止";
}

QString SerialReceiverPlugin::name() const {
    return QStringLiteral("org_common_serial_receiver");
}

void SerialReceiverPlugin::handleReplayStateChanged(bool replayActive) {
    const bool previous = replayActive_.exchange(replayActive, std::memory_order_acq_rel);
    if (previous == replayActive) {
        return;
    }

    if (replayActive) {
        qInfo() << "[SERIAL] 进入重演状态，关闭串口接收";
        closeSerialPort();
    } else {
        qInfo() << "[SERIAL] 离开重演状态，恢复串口接收";
        openSerialPort();
    }
}

void SerialReceiverPlugin::openSerialPort() {
    if (runtimeOpened_ || portName_.isEmpty()) {
        return;
    }

    serialPort_ = std::make_unique<QSerialPort>();
    serialPort_->setPortName(portName_);
    serialPort_->setBaudRate(baudRate_);
    serialPort_->setDataBits(QSerialPort::Data8);
    serialPort_->setParity(QSerialPort::NoParity);
    serialPort_->setStopBits(QSerialPort::OneStop);
    serialPort_->setFlowControl(QSerialPort::NoFlowControl);

    QObject::connect(serialPort_.get(), &QSerialPort::readyRead, [this]() {
        if (!serialPort_) {
            return;
        }

        const QByteArray payload = serialPort_->readAll();
        if (payload.isEmpty()) {
            return;
        }

        publishRaw(payload);
        qDebug().noquote() << QStringLiteral("[SERIAL] 收到 %1 bytes").arg(payload.size());
    });

    if (!serialPort_->open(QIODevice::ReadOnly)) {
        qWarning() << "[SERIAL] 打开失败，端口=" << portName_ << ", 错误=" << serialPort_->errorString();
        serialPort_.reset();
        runtimeOpened_ = false;
        return;
    }

    runtimeOpened_ = true;
    qInfo() << "[SERIAL] 已打开串口" << portName_ << "baud=" << serialPort_->baudRate();
}

void SerialReceiverPlugin::closeSerialPort() {
    if (serialPort_) {
        serialPort_->close();
        serialPort_.reset();
    }
    runtimeOpened_ = false;
}

void SerialReceiverPlugin::publishRaw(const QByteArray& payload) const {
    if (replayActive_.load(std::memory_order_acquire)) {
        return;
    }

    auto* manager = pluginManager();
    if (manager == nullptr || manager->messageBus() == nullptr) {
        return;
    }

    manager->messageBus()->publish(QStringLiteral("serial/raw"), payload);
}
