#include "eddy_current_panel_activator.h"

#include "eddy_current_panel_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void EddyCurrentPanelActivator::start(ctkPluginContext* context) {
    qInfo() << "[EDDY_CURRENT_PANEL] CTK start";

    plugin_ = new EddyCurrentPanelPlugin();
    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[EDDY_CURRENT_PANEL] PluginManager injected";
    } else {
        qWarning() << "[EDDY_CURRENT_PANEL] PluginManager resolve failed";
    }

    plugin_->start();
}

void EddyCurrentPanelActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[EDDY_CURRENT_PANEL] CTK stop";
    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
