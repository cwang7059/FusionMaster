#include "fast_mirror_panel_plugin.h"

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
    panel.qmlSource = QStringLiteral("qrc:/org.common.fast.mirror.panel/qml/FastMirrorControlView.qml");
    panel.capabilityFilters = capabilityFilters;
    panel.order = order;
    return panel;
}

}  // namespace

void FastMirrorPanelPlugin::start() {
    qInfo() << "[FAST_MIRROR_PANEL] plugin started";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[FAST_MIRROR_PANEL] PluginManager unavailable";
        return;
    }

    manager->registerService<IDevicePanelProvider>(
        QStringLiteral("device_panel/%1").arg(name()),
        this);
}

void FastMirrorPanelPlugin::stop() {
    qInfo() << "[FAST_MIRROR_PANEL] plugin stopped";
}

QString FastMirrorPanelPlugin::name() const {
    return QStringLiteral("org_common_fast_mirror_panel");
}

QList<DevicePanelContribution> FastMirrorPanelPlugin::panels() const {
    const QVariantMap capabilityFilters{
        {QStringLiteral("actions"),
         QVariantList{
             QStringLiteral("fast_mirror.send.command"),
             QStringLiteral("fast_mirror.set.lock_zero"),
             QStringLiteral("fast_mirror.set.point"),
         }},
        {QStringLiteral("protocol.type"), QStringLiteral("rs422.fast_mirror")},
    };

    return {
        makePanel(
            QStringLiteral("org_common_fast_mirror_panel.fast_mirror_model_panel"),
            QStringLiteral("快反镜专用面板"),
            QStringLiteral("fast_mirror"),
            QStringLiteral("org_business_fast_mirror_device.model"),
            capabilityFilters,
            100)
    };
}
