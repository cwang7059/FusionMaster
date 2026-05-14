#include "serial_receiver_activator.h"

#include "serial_receiver_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void SerialReceiverActivator::start(ctkPluginContext* context) {
    qInfo() << "[SERIAL] CTK start";

    plugin_ = new SerialReceiverPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[SERIAL] PluginManager injected";
    } else {
        qWarning() << "[SERIAL] PluginManager resolve failed";
    }

    plugin_->start();
}

void SerialReceiverActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[SERIAL] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
