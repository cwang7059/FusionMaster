#include "device_console_plugin.h"

#include "device_console_controller.h"
#include <plugin_api/iplugin_manager.h>

#include <QDebug>

#include <qqml.h>

DeviceConsolePlugin::DeviceConsolePlugin() = default;
DeviceConsolePlugin::~DeviceConsolePlugin() = default;

void DeviceConsolePlugin::start() {
    qInfo() << "[DEVICE_CONSOLE] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[DEVICE_CONSOLE] PluginManager 不可用";
        return;
    }

    qInfo() << "[DEVICE_CONSOLE] start-step: create-controller";
    if (!controller_) {
        controller_ = std::make_unique<DeviceConsoleController>();
        controller_->setPluginManager(manager);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        qInfo() << "[DEVICE_CONSOLE] start-step: qmlRegisterSingletonInstance";
        qmlRegisterSingletonInstance("DeviceConsole", 1, 0, "DeviceConsole", controller_.get());
#else
        qInfo() << "[DEVICE_CONSOLE] start-step: qmlRegisterSingletonType";
        qmlRegisterSingletonType<DeviceConsoleController>("DeviceConsole", 1, 0, "DeviceConsole",
            [](QQmlEngine*, QJSEngine*) -> QObject* {
                return new DeviceConsoleController();
            });
#endif
    }

    qInfo() << "[DEVICE_CONSOLE] start-step: controller-start";
    controller_->start();

    qInfo() << "[DEVICE_CONSOLE] start-step: register-ui-service";
    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);

    qInfo() << "[DEVICE_CONSOLE] start-step: done";
}

void DeviceConsolePlugin::stop() {
    qInfo() << "[DEVICE_CONSOLE] 插件停止";
    if (controller_) {
        controller_->stop();
        controller_.reset();
    }
}

QString DeviceConsolePlugin::name() const {
    return QStringLiteral("org_common_device_console");
}

QList<UiContribution> DeviceConsolePlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("org_common_device_console.panel");
    contribution.title = QStringLiteral("设备控制台");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/DeviceConsoleView.qml");
    contribution.order = 880;
    contribution.screenIndex = 0;

    UiContribution rightContribution;
    rightContribution.id = QStringLiteral("org_common_device_console.right_panel");
    rightContribution.title = QStringLiteral("设备管理");
    rightContribution.region = QStringLiteral("right");
    rightContribution.qmlSource = QStringLiteral("qml/DeviceConsoleRightPanel.qml");
    rightContribution.order = 100;
    rightContribution.screenIndex = 0;

    return {contribution, rightContribution};
}
