#include "encoder_panel_activator.h"

#include "encoder_panel_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void EncoderPanelActivator::start(ctkPluginContext* context) {
    qInfo() << "[ENCODER_PANEL] CTK start";

    plugin_ = new EncoderPanelPlugin();
    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[ENCODER_PANEL] PluginManager injected";
    } else {
        qWarning() << "[ENCODER_PANEL] PluginManager resolve failed";
    }

    plugin_->start();
}

void EncoderPanelActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[ENCODER_PANEL] CTK stop";
    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
