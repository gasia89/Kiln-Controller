#include "TimeProportionalSsr.h"

TimeProportionalSsr::TimeProportionalSsr(uint8_t pin, uint32_t windowMs)
    : pin_(pin), windowMs_(windowMs) {}

void TimeProportionalSsr::begin() {
    pinMode(pin_, OUTPUT);
    forceOff();
    windowStartMs_ = millis();
}

void TimeProportionalSsr::setPowerPercent(float percent) {
    powerPercent_ = constrain(percent, 0.0F, 100.0F);
}

float TimeProportionalSsr::powerPercent() const {
    return powerPercent_;
}

void TimeProportionalSsr::update(uint32_t nowMs) {
    if (nowMs - windowStartMs_ >= windowMs_) {
        windowStartMs_ = nowMs;
    }

    const uint32_t onTime = static_cast<uint32_t>(windowMs_ * powerPercent_ / 100.0F);
    const bool shouldBeOn = powerPercent_ > 0.0F && (nowMs - windowStartMs_) < onTime;
    digitalWrite(pin_, shouldBeOn ? HIGH : LOW);
    isOn_ = shouldBeOn;
}

void TimeProportionalSsr::forceOff() {
    digitalWrite(pin_, LOW);
    isOn_ = false;
}

bool TimeProportionalSsr::isOn() const {
    return isOn_;
}
