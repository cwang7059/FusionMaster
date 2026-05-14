#include "fast_mirror_panel_activator.h"

#include "fast_mirror_panel_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void FastMirrorPanelActivator::start(ctkPluginContext* context) {
    qInfo() << "[FAST_MIRROR_PANEL] CTK start";

    plugin_ = new FastMirrorPanelPlugin();
    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[FAST_MIRROR_PANEL] PluginManager injected";
    } else {
        qWarning() << "[FAST_MIRROR_PANEL] PluginManager resolve failed";
    }

    plugin_->start();
}

void FastMirrorPanelActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[FAST_MIRROR_PANEL] CTK stop";
    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
