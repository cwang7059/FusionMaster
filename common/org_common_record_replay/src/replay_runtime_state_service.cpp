#include "replay_runtime_state_service.h"

ReplayRuntimeStateService::ReplayRuntimeStateService(QObject* parent)
    : QObject(parent) {}

bool ReplayRuntimeStateService::recordingActive() const {
    return recordingActive_;
}

bool ReplayRuntimeStateService::replayActive() const {
    return replayActive_;
}

void ReplayRuntimeStateService::setState(bool recordingActive, bool replayActive) {
    if (recordingActive_ == recordingActive && replayActive_ == replayActive) {
        return;
    }
    recordingActive_ = recordingActive;
    replayActive_ = replayActive;
    emit stateChanged();
}

