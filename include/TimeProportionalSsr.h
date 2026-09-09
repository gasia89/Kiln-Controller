#pragma once

#include <Arduino.h>

class TimeProportionalSsr final {
public:
    TimeProportionalSsr(uint8_t pin, uint32_t windowMs);

    void begin();
    void setPowerPercent(float percent);
    float powerPercent() const;
    void update(uint32_t nowMs);
    void forceOff();
    bool isOn() const;

private:
    uint8_t pin_;
    uint32_t windowMs_;
    float powerPercent_ = 0.0F;
    uint32_t windowStartMs_ = 0;
    bool isOn_ = false;
};
