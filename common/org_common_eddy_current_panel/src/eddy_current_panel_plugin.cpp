#include "eddy_current_panel_plugin.h"

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
    panel.qmlSource = QStringLiteral("qrc:/org.common.eddy.current.panel/qml/EddyCurrentControlView.qml");
    panel.capabilityFilters = capabilityFilters;
    panel.order = order;
    return panel;
}

}  // namespace

void EddyCurrentPanelPlugin::start() {
    qInfo() << "[EDDY_CURRENT_PANEL] plugin started";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[EDDY_CURRENT_PANEL] PluginManager unavailable";
        return;
    }

    manager->registerService<IDevicePanelProvider>(
        QStringLiteral("device_panel/%1").arg(name()),
        this);
}

void EddyCurrentPanelPlugin::stop() {
    qInfo() << "[EDDY_CURRENT_PANEL] plugin stopped";
}

QString EddyCurrentPanelPlugin::name() const {
    return QStringLiteral("org_common_eddy_current_panel");
}

QList<DevicePanelContribution> EddyCurrentPanelPlugin::panels() const {
    const QVariantMap capabilityFilters{
        {QStringLiteral("actions"),
         QVariantList{
             QStringLiteral("eddy55aa.read.version"),
             QStringLiteral("eddy55aa.read.frequency"),
             QStringLiteral("eddy55aa.read.protocol"),
         }},
        {QStringLiteral("protocol.type"), QStringLiteral("55aa.eddy_current")},
    };

    return {
        makePanel(
            QStringLiteral("org_common_eddy_current_panel.eddy_current_model_panel"),
            QStringLiteral("电涡流传感器面板"),
            QStringLiteral("eddy_current"),
            QStringLiteral("org_business_eddy_current_device.model"),
            capabilityFilters,
            100)
    };
}
