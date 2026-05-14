#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/iui_contribution.h>

#include <memory>

class RtspPlayerController;

class RtspPlayerPlugin : public IPlugin, public IUiContributionProvider {
public:
    RtspPlayerPlugin();
    ~RtspPlayerPlugin() override;

    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;

private:
    std::unique_ptr<RtspPlayerController> controller_;
};
