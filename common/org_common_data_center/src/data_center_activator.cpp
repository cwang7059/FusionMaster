#include "data_center_activator.h"

#include "data_center_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void DataCenterActivator::start(ctkPluginContext* context) {
    qInfo() << "[DATA_CENTER] CTK start";

    plugin_ = new DataCenterPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[DATA_CENTER] PluginManager injected";
    } else {
        qWarning() << "[DATA_CENTER] PluginManager resolve failed";
    }

    plugin_->start();
}

void DataCenterActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[DATA_CENTER] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}

