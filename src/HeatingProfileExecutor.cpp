#include "HeatingProfileExecutor.h"

#include <math.h>

bool HeatingProfileExecutor::start(const HeatingProfile& profile, float startingTemperatureCelsius, uint32_t nowMs) {
    if (!profile.validate().valid() || !isfinite(startingTemperatureCelsius)) {
        state_ = HeatingProfileRunState::Invalid;
        return false;
    }

    profile_ = profile;
    state_ = HeatingProfileRunState::Running;
    segmentIndex_ = 0;
    startingTemperatureCelsius_ = startingTemperatureCelsius;
    runStartMs_ = nowMs;
    segmentStartMs_ = nowMs;
    pausedElapsedMs_ = 0;
    return true;
}

void HeatingProfileExecutor::update(uint32_t nowMs) {
    if (state_ != HeatingProfileRunState::Running || segmentIndex_ >= profile_.segmentCount) {
        return;
    }

    while (state_ == HeatingProfileRunState::Running && activeElapsedSeconds(nowMs) >= segmentDurationSeconds()) {
        advanceSegment(nowMs);
    }
}

void HeatingProfileExecutor::pause(uint32_t nowMs) {
    if (state_ != HeatingProfileRunState::Running) {
        return;
    }
    pausedElapsedMs_ = nowMs - segmentStartMs_;
    state_ = HeatingProfileRunState::Paused;
}

void HeatingProfileExecutor::resume(uint32_t nowMs) {
    if (state_ != HeatingProfileRunState::Paused) {
        return;
    }
    segmentStartMs_ = nowMs - pausedElapsedMs_;
    state_ = HeatingProfileRunState::Running;
}

void HeatingProfileExecutor::stop() {
    if (state_ == HeatingProfileRunState::Running || state_ == HeatingProfileRunState::Paused) {
        state_ = HeatingProfileRunState::Stopped;
    }
}

HeatingProfileRunState HeatingProfileExecutor::state() const { return state_; }
size_t HeatingProfileExecutor::segmentIndex() const { return segmentIndex_; }

float HeatingProfileExecutor::targetCelsius() const {
    if (segmentIndex_ >= profile_.segmentCount) {
        return 0.0F;
    }
    return profile_.segments[segmentIndex_].targetCelsius;
}

uint32_t HeatingProfileExecutor::elapsedSeconds(uint32_t nowMs) const {
    const uint32_t elapsedMs = state_ == HeatingProfileRunState::Paused ? pausedElapsedMs_ : nowMs - runStartMs_;
    return elapsedMs / 1000U;
}

uint32_t HeatingProfileExecutor::remainingSeconds(uint32_t nowMs) const {
    const float remaining = totalDurationSeconds() - static_cast<float>(elapsedSeconds(nowMs));
    return remaining > 0.0F ? static_cast<uint32_t>(remaining) : 0U;
}

const HeatingProfile& HeatingProfileExecutor::profile() const { return profile_; }

float HeatingProfileExecutor::segmentStartingTemperature() const {
    float temperature = startingTemperatureCelsius_;
    for (size_t index = 0; index < segmentIndex_; ++index) {
        temperature = profile_.segments[index].targetCelsius;
    }
    return temperature;
}

float HeatingProfileExecutor::segmentDurationSeconds() const {
    if (segmentIndex_ >= profile_.segmentCount) {
        return 0.0F;
    }
    return profile_.segments[segmentIndex_].durationSeconds(segmentStartingTemperature());
}

float HeatingProfileExecutor::totalDurationSeconds() const {
    float total = 0.0F;
    float startingTemperature = startingTemperatureCelsius_;
    for (size_t index = 0; index < profile_.segmentCount; ++index) {
        total += profile_.segments[index].durationSeconds(startingTemperature);
        startingTemperature = profile_.segments[index].targetCelsius;
    }
    return total;
}

uint32_t HeatingProfileExecutor::activeElapsedSeconds(uint32_t nowMs) const {
    return (nowMs - segmentStartMs_) / 1000U;
}

void HeatingProfileExecutor::advanceSegment(uint32_t nowMs) {
    const uint32_t durationMs = static_cast<uint32_t>(segmentDurationSeconds() * 1000.0F);
    const uint32_t segmentElapsedMs = nowMs - segmentStartMs_;
    segmentStartMs_ += durationMs;
    if (segmentElapsedMs < durationMs) {
        segmentStartMs_ = nowMs;
    }
    ++segmentIndex_;
    if (segmentIndex_ >= profile_.segmentCount) {
        state_ = HeatingProfileRunState::Completed;
    }
}
