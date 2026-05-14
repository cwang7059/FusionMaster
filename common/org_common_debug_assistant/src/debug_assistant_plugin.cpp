#include "debug_assistant_plugin.h"

#include <plugin_api/iplugin_manager.h>

#include <QDebug>

void DebugAssistantPlugin::start() {
    qInfo() << "[DEBUG_ASSISTANT] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[DEBUG_ASSISTANT] PluginManager 不可用";
        return;
    }

    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);
}

void DebugAssistantPlugin::stop() {
    qInfo() << "[DEBUG_ASSISTANT] 插件停止";
}

QString DebugAssistantPlugin::name() const {
    return QStringLiteral("org_common_debug_assistant");
}

QList<UiContribution> DebugAssistantPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("org_common_debug_assistant.panel");
    contribution.title = QStringLiteral("调试助手");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/DebugAssistantView.qml");
    contribution.order = 900;
    contribution.screenIndex = 0;
    return {contribution};
}
