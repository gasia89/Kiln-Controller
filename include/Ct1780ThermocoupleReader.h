#pragma once

#include <Arduino.h>
#include "IThermocoupleReader.h"

class Ct1780ThermocoupleReader final : public IThermocoupleReader {
public:
    Ct1780ThermocoupleReader(uint8_t dataPin, size_t sensorCount);

    bool begin() override;
    size_t count() const override;
    bool read(size_t index, TemperatureSample& sample) override;

private:
    uint8_t dataPin_;
    size_t sensorCount_;
};
