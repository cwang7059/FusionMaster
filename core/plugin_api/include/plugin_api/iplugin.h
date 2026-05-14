#pragma once

#include <QString>

class IPluginManager;
class PluginRuntime;

class IPlugin {
public:
    virtual ~IPlugin() = default;

    // 插件生命周期由框架在主线程顺序调用。
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual QString name() const = 0;

    IPluginManager* pluginManager() const { return pluginManager_; }

    // 仅框架调用：在 start() 前注入 PluginManager。
    void setPluginManager(IPluginManager* manager) { pluginManager_ = manager; }

protected:
    // 由框架在 start() 之前注入。
    IPluginManager* pluginManager_ = nullptr;
    friend class PluginRuntime;
};
