#pragma once

#include <plugin_api/idevice_panel_provider.h>
#include <plugin_api/iplugin.h>

class EncoderPanelPlugin : public IPlugin, public IDevicePanelProvider {
public:
    void start() override;
    void stop() override;
    QString name() const override;

    QList<DevicePanelContribution> panels() const override;
};
