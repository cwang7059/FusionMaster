#pragma once

#include <ctkPluginActivator.h>

#include <QObject>

class NetReceiverPlugin;
class ctkPluginContext;

class NetReceiverActivator : public QObject, public ctkPluginActivator {
    Q_OBJECT
    Q_INTERFACES(ctkPluginActivator)
    Q_PLUGIN_METADATA(IID "org.commontk.pluginfw.pluginactivator")

public:
    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

private:
    NetReceiverPlugin* plugin_ = nullptr;
};
