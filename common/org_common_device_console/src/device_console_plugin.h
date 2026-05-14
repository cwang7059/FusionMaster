#pragma once

#include <plugin_api/iplugin.h>
#include <plugin_api/iui_contribution.h>

#include <memory>

class DeviceConsoleController;
class DeviceConsolePlugin : public IPlugin, public IUiContributionProvider {
public:
    DeviceConsolePlugin();
    ~DeviceConsolePlugin() override;

    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;

private:
    std::unique_ptr<DeviceConsoleController> controller_;
};
