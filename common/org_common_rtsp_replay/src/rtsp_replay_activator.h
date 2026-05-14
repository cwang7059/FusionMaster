#pragma once

#include <ctkPluginActivator.h>

#include <QObject>

class RtspReplayPlugin;
class ctkPluginContext;

class RtspReplayActivator : public QObject, public ctkPluginActivator {
    Q_OBJECT
    Q_INTERFACES(ctkPluginActivator)
    Q_PLUGIN_METADATA(IID "org.commontk.pluginfw.pluginactivator")

public:
    void start(ctkPluginContext* context) override;
    void stop(ctkPluginContext* context) override;

private:
    RtspReplayPlugin* plugin_ = nullptr;
};
