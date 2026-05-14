#include "bus_monitor_activator.h"

#include "bus_monitor_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void BusMonitorActivator::start(ctkPluginContext* context) {
    qInfo() << "[BUS_MONITOR] CTK start";

    plugin_ = new BusMonitorPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[BUS_MONITOR] PluginManager injected";
    } else {
        qWarning() << "[BUS_MONITOR] PluginManager resolve failed";
    }

    plugin_->start();
}

void BusMonitorActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[BUS_MONITOR] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
