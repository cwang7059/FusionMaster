#include "device_registry_plugin.h"

#include "device_registry_service.h"

#include <plugin_api/idevice_registry.h>
#include <plugin_api/iplugin_manager.h>

#include <QDebug>

DeviceRegistryPlugin::DeviceRegistryPlugin() = default;
DeviceRegistryPlugin::~DeviceRegistryPlugin() = default;

void DeviceRegistryPlugin::start() {
    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[DEVICE_REGISTRY] PluginManager 不可用";
        return;
    }

    service_ = std::make_unique<DeviceRegistryService>();
    manager->registerService<IDeviceRegistry>(QStringLiteral("device_registry"), service_.get());

    qInfo() << "[DEVICE_REGISTRY] 设备注册中心服务已注册";
}

void DeviceRegistryPlugin::stop() {
    service_.reset();
    qInfo() << "[DEVICE_REGISTRY] 插件已停止";
}

QString DeviceRegistryPlugin::name() const {
    return QStringLiteral("org_common_device_registry");
}
