#pragma once

#include <stddef.h>
#include <stdint.h>
#include "HeatingProfile.h"

enum class HeatingProfileRunState : uint8_t {
    Idle,
    Running,
    Paused,
    Completed,
    Stopped,
    Invalid
};

class HeatingProfileExecutor final {
public:
    bool start(const HeatingProfile& profile, float startingTemperatureCelsius, uint32_t nowMs);
    void update(uint32_t nowMs);
    void pause(uint32_t nowMs);
    void resume(uint32_t nowMs);
    void stop();

    HeatingProfileRunState state() const;
    size_t segmentIndex() const;
    float targetCelsius() const;
    uint32_t elapsedSeconds(uint32_t nowMs) const;
    uint32_t remainingSeconds(uint32_t nowMs) const;
    const HeatingProfile& profile() const;

private:
    float segmentStartingTemperature() const;
    float segmentDurationSeconds() const;
    float totalDurationSeconds() const;
    uint32_t activeElapsedSeconds(uint32_t nowMs) const;
    void advanceSegment(uint32_t nowMs);

    HeatingProfile profile_;
    HeatingProfileRunState state_ = HeatingProfileRunState::Idle;
    size_t segmentIndex_ = 0;
    float startingTemperatureCelsius_ = 0.0F;
    uint32_t runStartMs_ = 0;
    uint32_t segmentStartMs_ = 0;
    uint32_t pausedElapsedMs_ = 0;
};
