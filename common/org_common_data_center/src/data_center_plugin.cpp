#include "data_center_plugin.h"

#include <plugin_api/idata_model_center.h>
#include <plugin_api/iplugin_manager.h>

#include <QDebug>

void DataCenterPlugin::start() {
    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[DATA_CENTER] PluginManager 不可用";
        return;
    }

    dataModelCenterService_ = std::make_unique<DataModelCenterService>();
    manager->registerService<IDataModelCenter>(
        QStringLiteral("data_model_center"),
        dataModelCenterService_.get());

    qInfo() << "[DATA_CENTER] 数据模型中心服务已注册";
}

void DataCenterPlugin::stop() {
    dataModelCenterService_.reset();
    qInfo() << "[DATA_CENTER] 插件已停止";
}

QString DataCenterPlugin::name() const {
    return QStringLiteral("org_common_data_center");
}
