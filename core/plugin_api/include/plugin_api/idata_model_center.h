#pragma once

#include <plugin_api/idata_model.h>

#include <QString>
#include <QStringList>

#include <functional>

using ModelCenterUpdateCallback = std::function<void(const QString& modelId, ModelOp op, const QStringList& itemIds)>;

class IDataModelCenter {
public:
    virtual ~IDataModelCenter() = default;

    // 注册/替换模型，modelId 由 model->modelId() 提供
    virtual bool registerModel(IDataModel* model) = 0;
    virtual bool unregisterModel(const QString& modelId) = 0;

    virtual IDataModel* model(const QString& modelId) = 0;
    virtual QStringList modelIds() const = 0;

    // 统一模型更新通知：modelId + op + itemIds
    virtual ModelSubscriptionId subscribe(ModelCenterUpdateCallback callback) = 0;
    virtual void unsubscribe(ModelSubscriptionId id) = 0;
};
