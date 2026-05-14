#include "data_repository_plugin.h"

#include <plugin_api/idata_repository.h>
#include <plugin_api/iplugin_manager.h>

#include <QDebug>
#include <QVariantMap>

namespace {

QVariantMap sanitizedOptions(QVariantMap options) {
    if (options.contains(QStringLiteral("password"))) {
        options.insert(QStringLiteral("password"), QStringLiteral("***"));
    }
    return options;
}

}  // namespace

void DataRepositoryPlugin::start() {
    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[DATA_REPOSITORY] PluginManager 不可用";
        return;
    }

    dataRepositoryService_ = std::make_unique<DataRepositoryService>();

    QString errorMessage;
    if (dataRepositoryService_->initializeDefaultStorage(&errorMessage)) {
        qInfo() << "[DATA_REPOSITORY] MySQL 数据库已就绪"
                << sanitizedOptions(dataRepositoryService_->defaultOptions());
    } else {
        qWarning() << "[DATA_REPOSITORY] MySQL 数据库初始化失败"
                   << sanitizedOptions(dataRepositoryService_->defaultOptions())
                   << errorMessage;
    }

    manager->registerService<IDataRepository>(
        QStringLiteral("data_repository"),
        dataRepositoryService_.get());

    qInfo() << "[DATA_REPOSITORY] 数据仓储服务已注册";
}

void DataRepositoryPlugin::stop() {
    dataRepositoryService_.reset();
    qInfo() << "[DATA_REPOSITORY] 插件已停止";
}

QString DataRepositoryPlugin::name() const {
    return QStringLiteral("org_common_data_repository");
}
