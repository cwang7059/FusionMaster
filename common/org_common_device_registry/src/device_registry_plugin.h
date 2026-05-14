#pragma once

#include <plugin_api/iplugin.h>

#include <memory>

class DeviceRegistryService;

class DeviceRegistryPlugin : public IPlugin {
public:
    DeviceRegistryPlugin();
    ~DeviceRegistryPlugin() override;

    void start() override;
    void stop() override;
    QString name() const override;

private:
    std::unique_ptr<DeviceRegistryService> service_;
};
