#pragma once

#include <plugin_api/ctk_bridge.h>

#include <QObject>

class PluginManagerBridgeService final
    : public QObject
    , public plugin_api::ctk_bridge::IPluginManagerBridge {
    Q_OBJECT
    Q_INTERFACES(plugin_api::ctk_bridge::IPluginManagerBridge)

public:
    using QObject::QObject;
};
