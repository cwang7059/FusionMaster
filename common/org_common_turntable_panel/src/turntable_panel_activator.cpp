#include "turntable_panel_activator.h"

#include "turntable_panel_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void TurntablePanelActivator::start(ctkPluginContext* context) {
    qInfo() << "[TURNTABLE_PANEL] CTK start";

    plugin_ = new TurntablePanelPlugin();
    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[TURNTABLE_PANEL] PluginManager injected";
    } else {
        qWarning() << "[TURNTABLE_PANEL] PluginManager resolve failed";
    }

    plugin_->start();
}

void TurntablePanelActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[TURNTABLE_PANEL] CTK stop";
    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
