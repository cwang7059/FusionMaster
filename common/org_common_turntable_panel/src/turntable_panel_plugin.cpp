#include "turntable_panel_plugin.h"

#include <plugin_api/iplugin_manager.h>

#include <QDebug>

namespace {

DevicePanelContribution makePanel(
    const QString& id,
    const QString& title,
    const QString& deviceType,
    const QString& modelId,
    const QVariantMap& capabilityFilters,
    int order) {
    DevicePanelContribution panel;
    panel.id = id;
    panel.title = title;
    panel.deviceType = deviceType;
    panel.modelId = modelId;
    panel.qmlSource = QStringLiteral("qrc:/org.common.turntable.panel/qml/TurntableControlView.qml");
    panel.capabilityFilters = capabilityFilters;
    panel.order = order;
    return panel;
}

}  // namespace

void TurntablePanelPlugin::start() {
    qInfo() << "[TURNTABLE_PANEL] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[TURNTABLE_PANEL] PluginManager 不可用";
        return;
    }

    manager->registerService<IDevicePanelProvider>(
        QStringLiteral("device_panel/%1").arg(name()),
        this);
}

void TurntablePanelPlugin::stop() {
    qInfo() << "[TURNTABLE_PANEL] 插件停止";
}

QString TurntablePanelPlugin::name() const {
    return QStringLiteral("org_common_turntable_panel");
}

QList<DevicePanelContribution> TurntablePanelPlugin::panels() const {
    const QVariantMap capabilityFilters{
        {QStringLiteral("actions"),
         QVariantList{
             QStringLiteral("turntable.send.command"),
             QStringLiteral("turntable.send.stop"),
             QStringLiteral("turntable.send.enable"),
         }},
    };

    return {
        makePanel(
            QStringLiteral("org_common_turntable_panel.turntable_model_panel"),
            QStringLiteral("转台专用面板"),
            QStringLiteral("turntable"),
            QStringLiteral("org_business_turntable_device.model"),
            capabilityFilters,
            100)
    };
}
