#include "record_replay_activator.h"

#include "record_replay_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void RecordReplayActivator::start(ctkPluginContext* context) {
    qInfo() << "[RECORD_REPLAY] CTK start";

    plugin_ = new RecordReplayPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[RECORD_REPLAY] PluginManager injected";
    } else {
        qWarning() << "[RECORD_REPLAY] PluginManager resolve failed";
    }

    plugin_->start();
}

void RecordReplayActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[RECORD_REPLAY] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}

