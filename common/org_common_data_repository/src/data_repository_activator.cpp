#include "data_repository_activator.h"

#include "data_repository_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void DataRepositoryActivator::start(ctkPluginContext* context) {
    qInfo() << "[DATA_REPOSITORY] CTK start";

    plugin_ = new DataRepositoryPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[DATA_REPOSITORY] PluginManager injected";
    } else {
        qWarning() << "[DATA_REPOSITORY] PluginManager resolve failed";
    }

    plugin_->start();
}

void DataRepositoryActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[DATA_REPOSITORY] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
