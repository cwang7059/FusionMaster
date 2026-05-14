#pragma once

#include <QVariantMap>

#include <memory>

class FastMirrorProtocolBridge {
public:
    FastMirrorProtocolBridge();
    ~FastMirrorProtocolBridge();

    QVariantMap capabilities() const;

    bool connectSerial(const QVariantMap& options);
    bool connectUdp(const QVariantMap& options);
    void disconnectDevice();
    bool isConnected() const;

    bool invoke(const QString& action, const QVariantMap& args);
    QVariantMap snapshot() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
