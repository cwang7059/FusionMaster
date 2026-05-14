#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/iui_contribution.h>

#include <plugin_api/imessage_bus.h>

class BusMonitorController;

class BusMonitorPlugin : public IPlugin, public IUiContributionProvider {
public:
    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;

private:
    BusMonitorController* controller_ = nullptr;
    SubscriptionId subscriptionId_ = 0;
    IMessageBus* messageBus_ = nullptr;
};
