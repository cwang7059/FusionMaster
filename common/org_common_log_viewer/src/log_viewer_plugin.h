#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/iui_contribution.h>

class LogViewerController;

class LogViewerPlugin : public IPlugin, public IUiContributionProvider {
public:
    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;

private:
    LogViewerController* controller_ = nullptr;
};
