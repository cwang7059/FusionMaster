#include "system_overview_plugin.h"

#include <plugin_api/iplugin_manager.h>

#include <QDebug>

void SystemOverviewPlugin::start() {
    qInfo() << "[SYSTEM_OVERVIEW] plugin start";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[SYSTEM_OVERVIEW] PluginManager unavailable";
        return;
    }

    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);
}

void SystemOverviewPlugin::stop() {
    qInfo() << "[SYSTEM_OVERVIEW] plugin stop";
}

QString SystemOverviewPlugin::name() const {
    return QStringLiteral("org_business_system_overview");
}

QList<UiContribution> SystemOverviewPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("system_overview.main_tab");
    contribution.title = QStringLiteral("首页总览");
    contribution.surface = QStringLiteral("main_tab");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/SystemOverviewView.qml");
    contribution.order = 100;
    contribution.screenIndex = 0;
    contribution.navText = QStringLiteral("首页总览");
    contribution.navIcon = QStringLiteral("\u2302");
    contribution.navOrder = 100;
    return {contribution};
}
