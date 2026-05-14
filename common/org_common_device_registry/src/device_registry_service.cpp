#include "device_registry_service.h"

#include <plugin_api/idevice_adapter.h>

#include <QMutexLocker>

bool DeviceRegistryService::registerAdapter(IDeviceAdapter* adapter) {
    if (adapter == nullptr) {
        return false;
    }

    const QString id = adapter->deviceId().trimmed();
    if (id.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&mutex_);
    adapters_.insert(id, adapter);
    return true;
}

bool DeviceRegistryService::unregisterAdapter(const QString& deviceId) {
    const QString id = deviceId.trimmed();
    if (id.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&mutex_);
    return adapters_.remove(id) > 0;
}

QStringList DeviceRegistryService::deviceIds() const {
    QMutexLocker locker(&mutex_);
    return adapters_.keys();
}

IDeviceAdapter* DeviceRegistryService::adapter(const QString& deviceId) {
    QMutexLocker locker(&mutex_);
    return adapters_.value(deviceId.trimmed(), nullptr);
}
