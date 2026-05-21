#include "system_overview_activator.h"

#include "system_overview_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void SystemOverviewActivator::start(ctkPluginContext* context) {
    qInfo() << "[SYSTEM_OVERVIEW] CTK start";

    plugin_ = new SystemOverviewPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[SYSTEM_OVERVIEW] PluginManager injected";
    } else {
        qWarning() << "[SYSTEM_OVERVIEW] PluginManager resolve failed";
    }

    plugin_->start();
}

void SystemOverviewActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[SYSTEM_OVERVIEW] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
