#include "log_viewer_activator.h"

#include "log_viewer_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void LogViewerActivator::start(ctkPluginContext* context) {
    qInfo() << "[LOG_VIEWER] CTK start";

    plugin_ = new LogViewerPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[LOG_VIEWER] PluginManager injected";
    } else {
        qWarning() << "[LOG_VIEWER] PluginManager resolve failed";
    }

    plugin_->start();
}

void LogViewerActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[LOG_VIEWER] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
