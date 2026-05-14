#include "record_replay_plugin.h"

#include "record_replay_controller.h"
#include "replay_runtime_state_service.h"

#include <plugin_api/iplugin_manager.h>
#include <plugin_api/ireplay_runtime_state.h>

#include <QDebug>
#include <QObject>
#include <QStringList>
#include <QTimer>

void RecordReplayPlugin::start() {
    qInfo() << "[RECORD_REPLAY] 插件启动";

    auto* manager = pluginManager();
    if (manager == nullptr) {
        qWarning() << "[RECORD_REPLAY] PluginManager 不可用";
        return;
    }

    controller_ = new RecordReplayController();
    replayStateService_ = new ReplayRuntimeStateService(controller_);
    replayStateService_->setState(controller_->recording(), controller_->replaying());
    QObject::connect(controller_, &RecordReplayController::runtimeChanged, replayStateService_, [this]() {
        if (controller_ == nullptr || replayStateService_ == nullptr) {
            return;
        }
        replayStateService_->setState(controller_->recording(), controller_->replaying());
    });
    rtspProbeTimer_ = nullptr;

    messageBus_ = manager->messageBus();
    controller_->setMessageBus(messageBus_);
    if (messageBus_ != nullptr) {
        const QStringList externalTopicPrefixes = {
            QStringLiteral("net/"),
            QStringLiteral("serial/"),
            QStringLiteral("can/"),
            QStringLiteral("rtsp/")
        };

        for (const QString& prefix : externalTopicPrefixes) {
            const SubscriptionId id = messageBus_->subscribeWithTopic(
                prefix,
                [this](const QString& topic, const QByteArray& payload) {
                    if (controller_ != nullptr) {
                        controller_->onBusMessage(topic, payload, QStringLiteral("runtime"));
                    }
                });
            if (id != 0) {
                subscriptionIds_.push_back(id);
            }
        }
    }

    manager->registerService<QObject>(
        QStringLiteral("controller/%1").arg(name()),
        controller_);

    manager->registerService<IReplayRuntimeState>(
        QStringLiteral("replay_runtime_state"),
        replayStateService_);

    manager->registerService<IUiContributionProvider>(
        QStringLiteral("ui/%1").arg(name()),
        this);

    auto syncRtspAvailability = [this, manager]() {
        if (controller_ == nullptr || manager == nullptr) {
            return;
        }
        const bool available =
            manager->getService<QObject>(QStringLiteral("controller/org_common_rtsp_replay")) != nullptr;
        controller_->setRtspAvailable(available);
    };

    syncRtspAvailability();
    rtspProbeTimer_ = new QTimer(controller_);
    rtspProbeTimer_->setInterval(1000);
    QObject::connect(rtspProbeTimer_, &QTimer::timeout, controller_, syncRtspAvailability);
    rtspProbeTimer_->start();
}

void RecordReplayPlugin::stop() {
    qInfo() << "[RECORD_REPLAY] 插件停止";

    if (messageBus_ != nullptr) {
        for (const SubscriptionId id : subscriptionIds_) {
            if (id != 0) {
                messageBus_->unsubscribe(id);
            }
        }
    }
    subscriptionIds_.clear();
    messageBus_ = nullptr;
    rtspProbeTimer_ = nullptr;

    if (replayStateService_ != nullptr) {
        replayStateService_->setState(false, false);
    }
    delete controller_;
    controller_ = nullptr;
    replayStateService_ = nullptr;
}

QString RecordReplayPlugin::name() const {
    return QStringLiteral("org_common_record_replay");
}

QList<UiContribution> RecordReplayPlugin::contributions() const {
    UiContribution contribution;
    contribution.id = QStringLiteral("org_common_record_replay.panel");
    contribution.title = QStringLiteral("记录重演");
    contribution.region = QStringLiteral("center");
    contribution.qmlSource = QStringLiteral("qml/RecordReplayView.qml");
    contribution.order = 915;
    contribution.screenIndex = 0;
    return {contribution};
}
