#pragma once

#include <plugin_api/ireplay_runtime_state.h>

#include <QObject>

class ReplayRuntimeStateService final : public QObject, public IReplayRuntimeState {
    Q_OBJECT
    Q_PROPERTY(bool recordingActive READ recordingActive NOTIFY stateChanged)
    Q_PROPERTY(bool replayActive READ replayActive NOTIFY stateChanged)

public:
    explicit ReplayRuntimeStateService(QObject* parent = nullptr);

    bool recordingActive() const override;
    bool replayActive() const override;

    void setState(bool recordingActive, bool replayActive);

signals:
    void stateChanged();

private:
    bool recordingActive_ = false;
    bool replayActive_ = false;
};

