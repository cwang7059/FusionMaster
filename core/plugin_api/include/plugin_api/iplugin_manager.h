#pragma once

#include <QString>
#include <QStringList>

#include <typeinfo>

class IMessageBus;
class ILogger;

class IPluginManager {
public:
    virtual ~IPluginManager() = default;

    virtual IMessageBus* messageBus() = 0;
    virtual ILogger* logger() = 0;

    // 通过名称和类型注册服务，业务插件只调用模板接口即可。
    template <typename T>
    void registerService(const QString& name, T* service) {
        registerServiceInternal(name, typeid(T).name(), static_cast<void*>(service));
    }

    // 通过名称和类型获取服务，获取失败返回 nullptr。
    template <typename T>
    T* getService(const QString& name) {
        return static_cast<T*>(getServiceInternal(name, typeid(T).name()));
    }

    template <typename T>
    QStringList findServiceNames(const QString& prefix = QString()) const {
        return findServiceNamesInternal(prefix, typeid(T).name());
    }

protected:
    virtual void registerServiceInternal(const QString& name, const char* typeId, void* service) = 0;
    virtual void* getServiceInternal(const QString& name, const char* typeId) = 0;
    virtual QStringList findServiceNamesInternal(const QString& prefix, const char* typeId) const = 0;
};
