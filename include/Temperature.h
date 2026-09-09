#pragma once

#include <stdint.h>

struct TemperatureSample {
    bool valid = false;
    float celsius = 0.0F;

    float fahrenheit() const {
        return (celsius * 9.0F / 5.0F) + 32.0F;
    }
};
