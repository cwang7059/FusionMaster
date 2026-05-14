#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/iui_contribution.h>

#include <plugin_api/imessage_bus.h>
#include <QList>

class RecordReplayController;
class QTimer;
class ReplayRuntimeStateService;

class RecordReplayPlugin : public IPlugin, public IUiContributionProvider {
public:
    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;

private:
    RecordReplayController* controller_ = nullptr;
    ReplayRuntimeStateService* replayStateService_ = nullptr;
    QList<SubscriptionId> subscriptionIds_;
    IMessageBus* messageBus_ = nullptr;
    QTimer* rtspProbeTimer_ = nullptr;
};

