#pragma once

#include <QtPlugin>

namespace plugin_api::ctk_bridge {

// CTK 服务类名：用于在动态插件中定位 PluginManager 桥接服务。
inline constexpr const char* kPluginManagerBridgeServiceClass = "osgi.plugin_manager.bridge";

// CTK 服务对象属性名：保存 PluginManager 指针（qulonglong）。
inline constexpr const char* kPluginManagerPointerProperty = "osgi.plugin_manager.ptr";

// 空接口，仅用于让 CTK 识别该 QObject 属于 bridge 服务类型。
class IPluginManagerBridge {
public:
    virtual ~IPluginManagerBridge() = default;
};

}  // namespace plugin_api::ctk_bridge

Q_DECLARE_INTERFACE(plugin_api::ctk_bridge::IPluginManagerBridge,
                    "osgi.plugin_manager.bridge")
