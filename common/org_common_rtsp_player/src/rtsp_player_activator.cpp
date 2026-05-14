#include "rtsp_player_activator.h"

#include "rtsp_player_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void RtspPlayerActivator::start(ctkPluginContext* context) {
    qInfo() << "[RTSP_PLAYER] CTK start";

    plugin_ = new RtspPlayerPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[RTSP_PLAYER] PluginManager injected";
    } else {
        qWarning() << "[RTSP_PLAYER] PluginManager resolve failed";
    }

    plugin_->start();
}

void RtspPlayerActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[RTSP_PLAYER] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
