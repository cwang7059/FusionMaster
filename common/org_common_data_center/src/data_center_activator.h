#pragma once

#include <ctkPluginActivator.h>

#include <QObject>

class DataCenterPlugin;
class ctkPluginContext;

class DataCenterActivator : public QObject, public ctkPluginActivator {
    Q_OBJECT
    Q_INTERFACES(ctkPluginActivator)
    Q_PLUGIN_METADATA(IID "org.commontk.pluginfw.pluginactivator")

public:
    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

private:
    DataCenterPlugin* plugin_ = nullptr;
};
