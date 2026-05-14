#include "device_console_activator.h"

#include "device_console_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void DeviceConsoleActivator::start(ctkPluginContext* context) {
    qInfo() << "[DEVICE_CONSOLE] CTK start";

    plugin_ = new DeviceConsolePlugin();
    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[DEVICE_CONSOLE] PluginManager injected";
    } else {
        qWarning() << "[DEVICE_CONSOLE] PluginManager resolve failed";
    }

    plugin_->start();
}

void DeviceConsoleActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[DEVICE_CONSOLE] CTK stop";
    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
