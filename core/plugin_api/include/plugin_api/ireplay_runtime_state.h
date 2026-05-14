#pragma once

class IReplayRuntimeState {
public:
    virtual ~IReplayRuntimeState() = default;

    virtual bool recordingActive() const = 0;
    virtual bool replayActive() const = 0;
};

