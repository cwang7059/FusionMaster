#pragma once

#include "data_model_center_service.h"

#include <plugin_api/iplugin.h>

#include <memory>

class DataCenterPlugin : public IPlugin {
public:
    void start() override;
    void stop() override;
    QString name() const override;

private:
    std::unique_ptr<DataModelCenterService> dataModelCenterService_;
};

