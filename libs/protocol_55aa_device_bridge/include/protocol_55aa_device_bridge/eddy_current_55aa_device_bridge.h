#pragma once

#include <QVariantMap>

#include <memory>

class EddyCurrent55aaDeviceBridge {
public:
    EddyCurrent55aaDeviceBridge();
    ~EddyCurrent55aaDeviceBridge();

    QVariantMap capabilities() const;

    bool connectSerial(const QVariantMap& options);
    void disconnectDevice();
    bool isConnected() const;

    bool invoke(const QString& action, const QVariantMap& args);
    QVariantMap snapshot() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
