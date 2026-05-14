#include "device_registry_activator.h"

#include "device_registry_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void DeviceRegistryActivator::start(ctkPluginContext* context) {
    qInfo() << "[DEVICE_REGISTRY] CTK start";

    plugin_ = new DeviceRegistryPlugin();
    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[DEVICE_REGISTRY] PluginManager injected";
    } else {
        qWarning() << "[DEVICE_REGISTRY] PluginManager resolve failed";
    }

    plugin_->start();
}

void DeviceRegistryActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[DEVICE_REGISTRY] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
