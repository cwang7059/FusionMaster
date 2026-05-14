#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/imessage_bus.h>

#include <QSerialPort>
#include <QString>

#include <atomic>
#include <memory>

class SerialReceiverPlugin : public IPlugin {
public:
    void start() override;
    void stop() override;
    QString name() const override;

private:
    void handleReplayStateChanged(bool replayActive);
    void openSerialPort();
    void closeSerialPort();
    void publishRaw(const QByteArray& payload) const;

private:
    std::unique_ptr<QSerialPort> serialPort_;
    QString portName_;
    qint32 baudRate_ = 115200;
    bool runtimeOpened_ = false;
    SubscriptionId replayStatusSubscriptionId_ = 0;
    std::atomic<bool> replayActive_{false};
};
