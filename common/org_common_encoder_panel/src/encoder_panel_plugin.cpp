#include "encoder_panel_plugin.h"

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
    panel.qmlSource = QStringLiteral("qrc:/org.common.encoder.panel/qml/EncoderControlView.qml");
    panel.capabilityFilters = capabilityFilters;
    panel.order = order;
    return panel;
}

}  // namespace

void EncoderPanelPlugin::start() {
    qInfo() << "[ENCODER_PANEL] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[ENCODER_PANEL] PluginManager 不可用";
        return;
    }

    manager->registerService<IDevicePanelProvider>(
        QStringLiteral("device_panel/%1").arg(name()),
        this);
}

void EncoderPanelPlugin::stop() {
    qInfo() << "[ENCODER_PANEL] 插件停止";
}

QString EncoderPanelPlugin::name() const {
    return QStringLiteral("org_common_encoder_panel");
}

QList<DevicePanelContribution> EncoderPanelPlugin::panels() const {
    const QVariantMap encoder55aaFilters{
        {QStringLiteral("actions"),
         QVariantList{
             QStringLiteral("encoder55aa.read.version"),
             QStringLiteral("encoder55aa.read.resolution"),
             QStringLiteral("encoder55aa.read.protocol"),
         }},
        {QStringLiteral("protocol.type"), QStringLiteral("55aa.encoder")},
    };

    return {
        makePanel(
            QStringLiteral("org_common_encoder_panel.encoder55aa_model_panel"),
            QStringLiteral("编码器 55AA 专用面板"),
            QStringLiteral("encoder"),
            QStringLiteral("org_business_encoder55aa_device.model"),
            encoder55aaFilters,
            100)
    };
}
