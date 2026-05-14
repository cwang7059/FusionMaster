#pragma once

#include <QHash>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <atomic>
#include <cstdint>
#include <functional>
#include <utility>

enum class ModelOp : std::uint8_t {
    Add = 0,
    Remove = 1,
    Update = 2,
    Reset = 3
};

using ModelSubscriptionId = std::uint64_t;
using ModelUpdateCallback = std::function<void(ModelOp op, const QStringList& itemIds)>;

class IDataModel {
public:
    virtual ~IDataModel() = default;

    virtual QString modelId() const = 0;

    // itemId 建议使用路径格式，例如 "motor.axis[0].position"
    virtual bool setValue(const QString& itemId, const QVariant& value) = 0;
    virtual bool removeValue(const QString& itemId) = 0;
    virtual QVariant value(const QString& itemId) const = 0;

    // 返回当前数据字典快照
    virtual QVariantMap snapshot() const = 0;

    // 全量替换数据字典，并触发 Reset
    virtual void reset(const QVariantMap& values) = 0;

    // 统一模型更新通知：op + 涉及的数据项 id 列表
    virtual ModelSubscriptionId subscribe(ModelUpdateCallback callback) = 0;
    virtual void unsubscribe(ModelSubscriptionId id) = 0;
};

// 默认数据模型基类
// 1) 使用字典存储数据项
// 2) 统一发出 modelUpdated(op, itemIds)
// 3) 提供对象子项、子列表的最小便捷操作
class DataModelBase : public IDataModel {
public:
    explicit DataModelBase(QString modelId)
        : modelId_(std::move(modelId)) {}

    QString modelId() const override {
        return modelId_;
    }

    bool setValue(const QString& itemId, const QVariant& value) override {
        if (itemId.trimmed().isEmpty()) {
            return false;
        }

        ModelOp op = ModelOp::Update;
        bool changed = false;
        {
            QMutexLocker locker(&mutex_);
            const auto it = values_.constFind(itemId);
            if (it == values_.constEnd()) {
                values_.insert(itemId, value);
                op = ModelOp::Add;
                changed = true;
            } else if (it.value() != value) {
                values_[itemId] = value;
                op = ModelOp::Update;
                changed = true;
            }
        }

        if (changed) {
            notify(op, QStringList{itemId});
        }
        return true;
    }

    bool removeValue(const QString& itemId) override {
        if (itemId.trimmed().isEmpty()) {
            return false;
        }

        bool removed = false;
        {
            QMutexLocker locker(&mutex_);
            removed = values_.remove(itemId) != 0;
        }

        if (removed) {
            notify(ModelOp::Remove, QStringList{itemId});
        }
        return removed;
    }

    QVariant value(const QString& itemId) const override {
        QMutexLocker locker(&mutex_);
        return values_.value(itemId);
    }

    QVariantMap snapshot() const override {
        QMutexLocker locker(&mutex_);
        QVariantMap map;
        for (auto it = values_.cbegin(); it != values_.cend(); ++it) {
            map.insert(it.key(), it.value());
        }
        return map;
    }

    void reset(const QVariantMap& values) override {
        {
            QMutexLocker locker(&mutex_);
            values_.clear();
            for (auto it = values.cbegin(); it != values.cend(); ++it) {
                values_.insert(it.key(), it.value());
            }
        }

        notify(ModelOp::Reset, values.keys());
    }

    // 将 itemId 对应数据项作为对象（QVariantMap）整体写入
    bool setObject(const QString& itemId, const QVariantMap& object) {
        return setValue(itemId, object);
    }

    // 将 itemId 对应数据项作为列表（QVariantList）整体写入
    bool setList(const QString& itemId, const QVariantList& list) {
        return setValue(itemId, list);
    }

    // 设置对象子项：objectId.childKey = value
    bool setChildValue(const QString& objectId, const QString& childKey, const QVariant& value) {
        const QString normalizedObjectId = objectId.trimmed();
        const QString normalizedChildKey = childKey.trimmed();
        if (normalizedObjectId.isEmpty() || normalizedChildKey.isEmpty()) {
            return false;
        }

        QVariantMap object = this->value(normalizedObjectId).toMap();
        object.insert(normalizedChildKey, value);
        return setValue(normalizedObjectId, object);
    }

    // 读取对象子项
    QVariant childValue(const QString& objectId, const QString& childKey) const {
        const QString normalizedObjectId = objectId.trimmed();
        const QString normalizedChildKey = childKey.trimmed();
        if (normalizedObjectId.isEmpty() || normalizedChildKey.isEmpty()) {
            return QVariant();
        }

        return this->value(normalizedObjectId).toMap().value(normalizedChildKey);
    }

    // 列表追加子项：listId.push_back(item)
    bool appendListItem(const QString& listId, const QVariant& item) {
        const QString normalizedListId = listId.trimmed();
        if (normalizedListId.isEmpty()) {
            return false;
        }

        QVariantList list = this->value(normalizedListId).toList();
        list.push_back(item);
        return setValue(normalizedListId, list);
    }

    // 更新列表指定索引项：listId[index] = item
    bool updateListItem(const QString& listId, int index, const QVariant& item) {
        const QString normalizedListId = listId.trimmed();
        if (normalizedListId.isEmpty()) {
            return false;
        }

        QVariantList list = this->value(normalizedListId).toList();
        if (index < 0 || index >= list.size()) {
            return false;
        }
        list[index] = item;
        return setValue(normalizedListId, list);
    }

    // 删除列表指定索引项
    bool removeListItem(const QString& listId, int index) {
        const QString normalizedListId = listId.trimmed();
        if (normalizedListId.isEmpty()) {
            return false;
        }

        QVariantList list = this->value(normalizedListId).toList();
        if (index < 0 || index >= list.size()) {
            return false;
        }
        list.removeAt(index);
        return setValue(normalizedListId, list);
    }

    // 对象中的子列表追加元素：objectId.childListKey.push_back(item)
    bool appendChildListItem(const QString& objectId, const QString& childListKey, const QVariant& item) {
        const QString normalizedObjectId = objectId.trimmed();
        const QString normalizedChildListKey = childListKey.trimmed();
        if (normalizedObjectId.isEmpty() || normalizedChildListKey.isEmpty()) {
            return false;
        }

        QVariantMap object = this->value(normalizedObjectId).toMap();
        QVariantList list = object.value(normalizedChildListKey).toList();
        list.push_back(item);
        object.insert(normalizedChildListKey, list);
        return setValue(normalizedObjectId, object);
    }

    // 更新对象中的子列表指定索引项
    bool updateChildListItem(const QString& objectId, const QString& childListKey, int index, const QVariant& item) {
        const QString normalizedObjectId = objectId.trimmed();
        const QString normalizedChildListKey = childListKey.trimmed();
        if (normalizedObjectId.isEmpty() || normalizedChildListKey.isEmpty()) {
            return false;
        }

        QVariantMap object = this->value(normalizedObjectId).toMap();
        QVariantList list = object.value(normalizedChildListKey).toList();
        if (index < 0 || index >= list.size()) {
            return false;
        }
        list[index] = item;
        object.insert(normalizedChildListKey, list);
        return setValue(normalizedObjectId, object);
    }

    // 删除对象中的子列表指定索引项
    bool removeChildListItem(const QString& objectId, const QString& childListKey, int index) {
        const QString normalizedObjectId = objectId.trimmed();
        const QString normalizedChildListKey = childListKey.trimmed();
        if (normalizedObjectId.isEmpty() || normalizedChildListKey.isEmpty()) {
            return false;
        }

        QVariantMap object = this->value(normalizedObjectId).toMap();
        QVariantList list = object.value(normalizedChildListKey).toList();
        if (index < 0 || index >= list.size()) {
            return false;
        }
        list.removeAt(index);
        object.insert(normalizedChildListKey, list);
        return setValue(normalizedObjectId, object);
    }

    ModelSubscriptionId subscribe(ModelUpdateCallback callback) override {
        if (!callback) {
            return 0;
        }

        QMutexLocker locker(&mutex_);
        const ModelSubscriptionId id = nextSubscriptionId_++;
        callbacks_.insert(id, std::move(callback));
        return id;
    }

    void unsubscribe(ModelSubscriptionId id) override {
        if (id == 0) {
            return;
        }
        QMutexLocker locker(&mutex_);
        callbacks_.remove(id);
    }

protected:
    void notify(ModelOp op, const QStringList& itemIds) {
        QList<ModelUpdateCallback> callbacksCopy;
        {
            QMutexLocker locker(&mutex_);
            callbacksCopy.reserve(callbacks_.size());
            for (auto it = callbacks_.cbegin(); it != callbacks_.cend(); ++it) {
                callbacksCopy.push_back(it.value());
            }
        }

        onModelUpdated(op, itemIds);
        for (const auto& callback : callbacksCopy) {
            callback(op, itemIds);
        }
    }

    // 子类可覆盖该钩子扩展行为（默认空实现）
    virtual void onModelUpdated(ModelOp op, const QStringList& itemIds) {
        Q_UNUSED(op)
        Q_UNUSED(itemIds)
    }

private:
    mutable QMutex mutex_;
    QString modelId_;
    QHash<QString, QVariant> values_;
    QHash<ModelSubscriptionId, ModelUpdateCallback> callbacks_;
    std::atomic<ModelSubscriptionId> nextSubscriptionId_ {1};
};
