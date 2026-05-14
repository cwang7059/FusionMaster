#pragma once

#include "data_repository_service.h"

#include <plugin_api/iplugin.h>

#include <memory>

class DataRepositoryPlugin : public IPlugin {
public:
    void start() override;
    void stop() override;
    QString name() const override;

private:
    std::unique_ptr<DataRepositoryService> dataRepositoryService_;
};
