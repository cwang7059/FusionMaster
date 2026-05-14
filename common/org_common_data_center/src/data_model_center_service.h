#pragma once

#include <plugin_api/idata_model_center.h>

#include <QHash>
#include <QMutex>
#include <QStringList>

#include <atomic>

class DataModelCenterService : public IDataModelCenter {
public:
    ~DataModelCenterService() override;

    bool registerModel(IDataModel* model) override;
    bool unregisterModel(const QString& modelId) override;

    IDataModel* model(const QString& modelId) override;
    QStringList modelIds() const override;

    ModelSubscriptionId subscribe(ModelCenterUpdateCallback callback) override;
    void unsubscribe(ModelSubscriptionId id) override;

private:
    struct ModelEntry {
        IDataModel* model = nullptr;
        ModelSubscriptionId modelSubscriptionId = 0;
    };

    void dispatchModelUpdated(const QString& modelId, ModelOp op, const QStringList& itemIds);

    mutable QMutex mutex_;
    QHash<QString, ModelEntry> models_;
    QHash<ModelSubscriptionId, ModelCenterUpdateCallback> callbacks_;
    std::atomic<ModelSubscriptionId> nextSubscriptionId_ {1};
};

