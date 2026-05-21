#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/iui_contribution.h>

class SystemOverviewPlugin : public IPlugin, public IUiContributionProvider {
public:
    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;
};
