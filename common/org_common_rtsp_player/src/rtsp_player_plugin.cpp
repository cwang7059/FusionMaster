#include "rtsp_player_plugin.h"

#include "rtsp_player_controller.h"
#include "rtsp_video_item.h"

#include <plugin_api/iplugin_manager.h>

#include <QDebug>

#include <qqml.h>

namespace {

void registerQmlTypes(RtspPlayerController* controller) {
    static bool videoItemRegistered = false;
    if (!videoItemRegistered) {
        qmlRegisterType<RtspVideoItem>("RtspPlayer", 1, 0, "RtspVideoItem");
        videoItemRegistered = true;
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    static bool singletonRegistered = false;
    if (!singletonRegistered) {
        qmlRegisterSingletonInstance("RtspPlayer", 1, 0, "RtspPlayerController", controller);
        singletonRegistered = true;
    }
#else
    Q_UNUSED(controller)
#endif
}

}  // namespace

RtspPlayerPlugin::RtspPlayerPlugin() = default;
RtspPlayerPlugin::~RtspPlayerPlugin() = default;

void RtspPlayerPlugin::start() {
    qInfo() << "[RTSP_PLAYER] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[RTSP_PLAYER] PluginManager 不可用";
        return;
    }

    if (!controller_) {
        controller_ = std::make_unique<RtspPlayerController>();
        controller_->setPluginManager(manager);
        registerQmlTypes(controller_.get());
    }

    controller_->start();

    manager->registerService<QObject>(
        QStringLiteral("controller/%1").arg(name()),
        controller_.get());

    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);
}

void RtspPlayerPlugin::stop() {
    qInfo() << "[RTSP_PLAYER] 插件停止";
    if (controller_) {
        controller_->stop();
        controller_.reset();
    }
}

QString RtspPlayerPlugin::name() const {
    return QStringLiteral("org_common_rtsp_player");
}

QList<UiContribution> RtspPlayerPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("org_common_rtsp_player.panel");
    contribution.title = QStringLiteral("RTSP播放器");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/RtspPlayerView.qml");
    contribution.order = 920;
    contribution.screenIndex = 0;
    return {contribution};
}
