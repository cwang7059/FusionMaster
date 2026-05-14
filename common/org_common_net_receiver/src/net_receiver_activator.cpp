#include "net_receiver_activator.h"

#include "net_receiver_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void NetReceiverActivator::start(ctkPluginContext* context) {
    qInfo() << "[NET] CTK start";

    plugin_ = new NetReceiverPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[NET] PluginManager injected";
    } else {
        qWarning() << "[NET] PluginManager resolve failed";
    }

    plugin_->start();
}

void NetReceiverActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[NET] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
