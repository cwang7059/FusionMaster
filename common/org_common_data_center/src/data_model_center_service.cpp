#include "data_model_center_service.h"

#include <QList>

DataModelCenterService::~DataModelCenterService() {
    QHash<QString, ModelEntry> modelsCopy;
    {
        QMutexLocker locker(&mutex_);
        modelsCopy = models_;
        models_.clear();
        callbacks_.clear();
    }

    for (auto it = modelsCopy.begin(); it != modelsCopy.end(); ++it) {
        if (it.value().model != nullptr && it.value().modelSubscriptionId != 0) {
            it.value().model->unsubscribe(it.value().modelSubscriptionId);
        }
    }
}

bool DataModelCenterService::registerModel(IDataModel* model) {
    if (model == nullptr) {
        return false;
    }

    const QString id = model->modelId().trimmed();
    if (id.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&mutex_);

    auto it = models_.find(id);
    if (it != models_.end()) {
        if (it.value().model == model) {
            return true;
        }
        if (it.value().model != nullptr && it.value().modelSubscriptionId != 0) {
            it.value().model->unsubscribe(it.value().modelSubscriptionId);
        }
    }

    ModelEntry entry;
    entry.model = model;
    entry.modelSubscriptionId = model->subscribe(
        [this, id](ModelOp op, const QStringList& itemIds) {
            dispatchModelUpdated(id, op, itemIds);
        });

    models_[id] = entry;
    return true;
}

bool DataModelCenterService::unregisterModel(const QString& modelId) {
    const QString id = modelId.trimmed();
    if (id.isEmpty()) {
        return false;
    }

    IDataModel* model = nullptr;
    ModelSubscriptionId modelSubscriptionId = 0;

    {
        QMutexLocker locker(&mutex_);
        auto it = models_.find(id);
        if (it == models_.end()) {
            return false;
        }
        model = it.value().model;
        modelSubscriptionId = it.value().modelSubscriptionId;
        models_.erase(it);
    }

    if (model != nullptr && modelSubscriptionId != 0) {
        model->unsubscribe(modelSubscriptionId);
    }
    return true;
}

IDataModel* DataModelCenterService::model(const QString& modelId) {
    const QString id = modelId.trimmed();
    if (id.isEmpty()) {
        return nullptr;
    }

    QMutexLocker locker(&mutex_);
    auto it = models_.find(id);
    if (it == models_.end()) {
        return nullptr;
    }
    return it.value().model;
}

QStringList DataModelCenterService::modelIds() const {
    QMutexLocker locker(&mutex_);
    return models_.keys();
}

ModelSubscriptionId DataModelCenterService::subscribe(ModelCenterUpdateCallback callback) {
    if (!callback) {
        return 0;
    }

    QMutexLocker locker(&mutex_);
    const ModelSubscriptionId id = nextSubscriptionId_++;
    callbacks_.insert(id, std::move(callback));
    return id;
}

void DataModelCenterService::unsubscribe(ModelSubscriptionId id) {
    if (id == 0) {
        return;
    }
    QMutexLocker locker(&mutex_);
    callbacks_.remove(id);
}

void DataModelCenterService::dispatchModelUpdated(const QString& modelId, ModelOp op, const QStringList& itemIds) {
    QList<ModelCenterUpdateCallback> callbacksCopy;
    {
        QMutexLocker locker(&mutex_);
        callbacksCopy.reserve(callbacks_.size());
        for (auto it = callbacks_.cbegin(); it != callbacks_.cend(); ++it) {
            callbacksCopy.push_back(it.value());
        }
    }

    for (const auto& callback : callbacksCopy) {
        callback(modelId, op, itemIds);
    }
}

