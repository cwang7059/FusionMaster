#pragma once

#include <plugin_api/idevice_registry.h>

#include <QHash>
#include <QMutex>

class DeviceRegistryService : public IDeviceRegistry {
public:
    bool registerAdapter(IDeviceAdapter* adapter) override;
    bool unregisterAdapter(const QString& deviceId) override;
    QStringList deviceIds() const override;
    IDeviceAdapter* adapter(const QString& deviceId) override;

private:
    mutable QMutex mutex_;
    QHash<QString, IDeviceAdapter*> adapters_;
};
