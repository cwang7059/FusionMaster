#include "history_data_fusion_analysis_activator.h"

#include "history_data_fusion_analysis_plugin.h"

#include <plugin_api/ctk_plugin_manager_resolver.h>

#include <ctkPluginContext.h>

#include <QDebug>

void HistoryDataFusionAnalysisActivator::start(ctkPluginContext* context) {
    qInfo() << "[HISTORY_FUSION] CTK start";

    plugin_ = new HistoryDataFusionAnalysisPlugin();

    if (auto* manager = plugin_api::ctk_bridge::resolvePluginManager(context); manager != nullptr) {
        plugin_->setPluginManager(manager);
        qInfo() << "[HISTORY_FUSION] PluginManager injected";
    } else {
        qWarning() << "[HISTORY_FUSION] PluginManager resolve failed";
    }

    plugin_->start();
}

void HistoryDataFusionAnalysisActivator::stop(ctkPluginContext* context) {
    Q_UNUSED(context)

    qInfo() << "[HISTORY_FUSION] CTK stop";

    if (plugin_ != nullptr) {
        plugin_->stop();
        delete plugin_;
        plugin_ = nullptr;
    }
}
