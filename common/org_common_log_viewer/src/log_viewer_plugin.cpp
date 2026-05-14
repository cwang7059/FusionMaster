#include "log_viewer_plugin.h"

#include "log_viewer_controller.h"

#include <logging/logging_service.h>
#include <plugin_api/iplugin_manager.h>

#include <QDebug>
#include <QObject>

void LogViewerPlugin::start() {
    qInfo() << "[LOG_VIEWER] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[LOG_VIEWER] PluginManager 不可用";
        return;
    }

    controller_ = new LogViewerController();

    auto* loggingService = manager->getService<LoggingService>(QStringLiteral("service/logging"));
    if (loggingService != nullptr) {
        controller_->bindLoggingService(loggingService);
    }

    manager->registerService<QObject>(
        QStringLiteral("controller/%1").arg(name()),
        controller_);

    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);
}

void LogViewerPlugin::stop() {
    qInfo() << "[LOG_VIEWER] 插件停止";

    delete controller_;
    controller_ = nullptr;
}

QString LogViewerPlugin::name() const {
    return QStringLiteral("org_common_log_viewer");
}

QList<UiContribution> LogViewerPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("org_common_log_viewer.panel");
    contribution.title = QStringLiteral("日志管理");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/LogViewerView.qml");
    contribution.order = 920;
    contribution.screenIndex = 0;
    return {contribution};
}
