#pragma once

#include <plugin_api/idata_model.h>
#include <plugin_api/iui_contribution.h>
#include <plugin_api/iplugin.h>

#include <memory>

class HistoryDataFusionController;
class IDataModelCenter;

class HistoryDataFusionAnalysisPlugin : public IPlugin, public IUiContributionProvider {
public:
    HistoryDataFusionAnalysisPlugin();
    ~HistoryDataFusionAnalysisPlugin() override;

    void start() override;
    void stop() override;
    QString name() const override;

    QList<UiContribution> contributions() const override;

private:
    class HistoryFusionDataModel;

    std::unique_ptr<HistoryDataFusionController> controller_;
    std::unique_ptr<HistoryFusionDataModel> model_;
    IDataModelCenter* dataModelCenter_ = nullptr;
};
