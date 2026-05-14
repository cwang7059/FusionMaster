#include "bus_monitor_plugin.h"

#include "bus_monitor_controller.h"

#include <plugin_api/iplugin_manager.h>

#include <QDebug>
#include <QObject>

void BusMonitorPlugin::start() {
    qInfo() << "[BUS_MONITOR] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[BUS_MONITOR] PluginManager 不可用";
        return;
    }

    controller_ = new BusMonitorController();

    messageBus_ = manager->messageBus();
    if (messageBus_ != nullptr) {
        subscriptionId_ = messageBus_->subscribeAll([this](const QString& topic, const QByteArray& data) {
            if (controller_ != nullptr) {
                controller_->appendBusMessage(topic, data, QStringLiteral("runtime_bus"));
            }
        });
    }

    manager->registerService<QObject>(
        QStringLiteral("controller/%1").arg(name()),
        controller_);

    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);
}

void BusMonitorPlugin::stop() {
    qInfo() << "[BUS_MONITOR] 插件停止";

    if (messageBus_ != nullptr && subscriptionId_ != 0) {
        messageBus_->unsubscribe(subscriptionId_);
    }
    subscriptionId_ = 0;
    messageBus_ = nullptr;

    delete controller_;
    controller_ = nullptr;
}

QString BusMonitorPlugin::name() const {
    return QStringLiteral("org_common_bus_monitor");
}

QList<UiContribution> BusMonitorPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("org_common_bus_monitor.panel");
    contribution.title = QStringLiteral("消息总线");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/BusMonitorView.qml");
    contribution.order = 910;
    contribution.screenIndex = 0;
    return {contribution};
}
