#pragma once

#include <QString>
#include <QStringList>

class IDeviceAdapter;

class IDeviceRegistry {
public:
    virtual ~IDeviceRegistry() = default;

    virtual bool registerAdapter(IDeviceAdapter* adapter) = 0;
    virtual bool unregisterAdapter(const QString& deviceId) = 0;
    virtual QStringList deviceIds() const = 0;
    virtual IDeviceAdapter* adapter(const QString& deviceId) = 0;
};
