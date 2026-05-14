#pragma once

#include <QVariantMap>
#include <QString>

class IDeviceAdapter {
public:
    virtual ~IDeviceAdapter() = default;

    virtual QString deviceId() const = 0;
    virtual QString displayName() const = 0;
    virtual QVariantMap capabilities() const = 0;

    virtual bool connectDevice(const QVariantMap& options) = 0;
    virtual void disconnectDevice() = 0;
    virtual bool isConnected() const = 0;

    virtual bool invoke(const QString& action, const QVariantMap& args) = 0;
    virtual QString modelId() const = 0;
};
