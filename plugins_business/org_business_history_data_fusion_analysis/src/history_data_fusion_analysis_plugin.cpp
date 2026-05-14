#include "history_data_fusion_analysis_plugin.h"

#include "history_data_fusion_controller.h"

#include <plugin_api/idata_model_center.h>
#include <plugin_api/idata_repository.h>
#include <plugin_api/iplugin_manager.h>

#include <QDebug>
#include <QQmlEngine>
#include <QtQml/qqml.h>

namespace {

HistoryDataFusionController* g_controller = nullptr;

void ensureQmlTypesRegistered() {
    static bool registered = false;
    if (registered) {
        return;
    }

    qmlRegisterSingletonType<HistoryDataFusionController>(
        "HistoryFusionAnalysis",
        1,
        0,
        "HistoryFusionController",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            QQmlEngine::setObjectOwnership(g_controller, QQmlEngine::CppOwnership);
            return g_controller;
        });

    registered = true;
}

}  // namespace

class HistoryDataFusionAnalysisPlugin::HistoryFusionDataModel : public DataModelBase {
public:
    HistoryFusionDataModel()
        : DataModelBase(QStringLiteral("org_business_history_data_fusion_analysis.model")) {
        setValue(QStringLiteral("plugin.name"), QStringLiteral("org_business_history_data_fusion_analysis"));
        setValue(QStringLiteral("task.state"), QStringLiteral("未启动"));
    }
};

HistoryDataFusionAnalysisPlugin::HistoryDataFusionAnalysisPlugin() = default;
HistoryDataFusionAnalysisPlugin::~HistoryDataFusionAnalysisPlugin() = default;

void HistoryDataFusionAnalysisPlugin::start() {
    qInfo() << "[HISTORY_FUSION] plugin start";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[HISTORY_FUSION] PluginManager unavailable";
        return;
    }

    if (!controller_) {
        controller_ = std::make_unique<HistoryDataFusionController>();
    }
    if (!model_) {
        model_ = std::make_unique<HistoryFusionDataModel>();
    }

    controller_->setMessageBus(manager->messageBus());
    controller_->setDataModel(model_.get());
    g_controller = controller_.get();
    ensureQmlTypesRegistered();

    dataModelCenter_ = manager->getService<IDataModelCenter>(QStringLiteral("data_model_center"));
    if (dataModelCenter_ != nullptr) {
        dataModelCenter_->registerModel(model_.get());
    } else {
        qWarning() << "[HISTORY_FUSION] data_model_center service not found";
    }

    auto* dataRepository = manager->getService<IDataRepository>(QStringLiteral("data_repository"));
    if (dataRepository != nullptr && dataRepository->isOpen()) {
        qInfo() << "[HISTORY_FUSION] data_repository service ready"
                << dataRepository->defaultOptions().value(QStringLiteral("databaseName")).toString();
    } else {
        qWarning() << "[HISTORY_FUSION] data_repository service not ready";
    }

    manager->registerService<QObject>(
        QStringLiteral("controller/%1").arg(name()),
        controller_.get());
    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);

    controller_->resetTask();
}

void HistoryDataFusionAnalysisPlugin::stop() {
    qInfo() << "[HISTORY_FUSION] plugin stop";

    if (model_ && dataModelCenter_ != nullptr) {
        dataModelCenter_->unregisterModel(model_->modelId());
    }

    g_controller = nullptr;
    controller_.reset();
    model_.reset();
    dataModelCenter_ = nullptr;
}

QString HistoryDataFusionAnalysisPlugin::name() const {
    return QStringLiteral("org_business_history_data_fusion_analysis");
}

QList<UiContribution> HistoryDataFusionAnalysisPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("history_data_fusion_analysis.panel");
    contribution.title = QStringLiteral("历史数据融合分析");
    contribution.surface = QStringLiteral("panel");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/HistoryDataFusionAnalysisView.qml");
    contribution.order = 330;
    contribution.screenIndex = 0;
    contribution.navText = QStringLiteral("融合分析");
    contribution.navIcon = QStringLiteral("\u2387");
    contribution.navOrder = 330;
    contribution.requiredServices = QStringList()
        << QStringLiteral("data_model_center")
        << QStringLiteral("data_repository");
    return {contribution};
}
