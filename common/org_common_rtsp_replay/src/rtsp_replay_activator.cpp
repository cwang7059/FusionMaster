#include "rtsp_replay_activator.h"

#include "rtsp_replay_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void RtspReplayActivator::start(ctkPluginContext* context) {
    qInfo() << "[RTSP_REPLAY] CTK start";

    plugin_ = new RtspReplayPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[RTSP_REPLAY] PluginManager injected";
    } else {
        qWarning() << "[RTSP_REPLAY] PluginManager resolve failed";
    }

    plugin_->start();
}

void RtspReplayActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[RTSP_REPLAY] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
