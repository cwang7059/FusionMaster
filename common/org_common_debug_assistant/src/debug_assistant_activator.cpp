#include "debug_assistant_activator.h"

#include "debug_assistant_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void DebugAssistantActivator::start(ctkPluginContext* context) {
    qInfo() << "[DEBUG_ASSISTANT] CTK start";

    plugin_ = new DebugAssistantPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[DEBUG_ASSISTANT] PluginManager injected";
    } else {
        qWarning() << "[DEBUG_ASSISTANT] PluginManager resolve failed";
    }

    plugin_->start();
}

void DebugAssistantActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[DEBUG_ASSISTANT] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
