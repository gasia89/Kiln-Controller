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
    static constexpr size_t kMaxDevices = 16;
    static constexpr uint8_t kCt1780FamilyCode = 0x3B;

    bool readScratchpad(const uint8_t* address, uint8_t* scratchpad);
    int readConfigAddress(const uint8_t* address);

    uint8_t dataPin_;
    size_t requestedSensorCount_;
    size_t deviceCount_ = 0;
    uint8_t addresses_[kMaxDevices][8] = {};
};
