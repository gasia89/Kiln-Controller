#include "Ct1780ThermocoupleReader.h"

#include <OneWire.h>
#include <string.h>

namespace {
constexpr uint8_t kConvertTemperature = 0x44;
constexpr uint8_t kReadScratchpad = 0xBE;
constexpr uint32_t kConversionTimeMs = 750;
constexpr float kMinimumTemperatureCelsius = -270.0F;
constexpr float kMaximumTemperatureCelsius = 1372.0F;

uint8_t crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    while (length-- > 0) {
        uint8_t value = *data++;
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint8_t mix = (crc ^ value) & 0x01;
            crc >>= 1;
            if (mix != 0) {
                crc ^= 0x8C;
            }
            value >>= 1;
        }
    }
    return crc;
}

bool allBytesEqual(const uint8_t* data, size_t length) {
    for (size_t index = 1; index < length; ++index) {
        if (data[index] != data[0]) {
            return false;
        }
    }
    return true;
}
}

Ct1780ThermocoupleReader::Ct1780ThermocoupleReader(uint8_t dataPin, size_t sensorCount)
    : dataPin_(dataPin), requestedSensorCount_(sensorCount) {}

bool Ct1780ThermocoupleReader::begin() {
    pinMode(dataPin_, INPUT_PULLUP);
    deviceCount_ = 0;
    if (requestedSensorCount_ == 0 || requestedSensorCount_ > kMaxDevices) {
        return false;
    }

    OneWire bus(dataPin_);
    uint8_t address[8] = {};
    bus.reset_search();
    while (bus.search(address)) {
        if (address[0] != kCt1780FamilyCode || crc8(address, 7) != address[7]) {
            continue;
        }
        if (deviceCount_ >= kMaxDevices) {
            break;
        }
        memcpy(addresses_[deviceCount_], address, sizeof(address));
        ++deviceCount_;
    }

    for (size_t index = 0; index < deviceCount_; ++index) {
        const int configAddress = readConfigAddress(addresses_[index]);
        if (configAddress < 0) {
            deviceCount_ = 0;
            return false;
        }
        for (size_t previous = 0; previous < index; ++previous) {
            if (readConfigAddress(addresses_[previous]) == configAddress) {
                deviceCount_ = 0;
                return false;
            }
        }
    }

    return deviceCount_ == requestedSensorCount_;
}

size_t Ct1780ThermocoupleReader::count() const {
    return deviceCount_;
}

bool Ct1780ThermocoupleReader::read(size_t index, TemperatureSample& sample) {
    sample = {};
    if (index >= deviceCount_) {
        return false;
    }

    OneWire bus(dataPin_);
    if (bus.reset() == 0) {
        return false;
    }
    bus.select(addresses_[index]);
    bus.write(kConvertTemperature);
    delay(kConversionTimeMs);

    uint8_t scratchpad[9] = {};
    if (!readScratchpad(addresses_[index], scratchpad)) {
        return false;
    }

    int16_t rawTemperature = static_cast<int16_t>((scratchpad[1] << 8) | scratchpad[0]);
    rawTemperature >>= 2;
    if ((rawTemperature & 0x2000) != 0) {
        rawTemperature |= static_cast<int16_t>(0xC000);
    }
    const float celsius = static_cast<float>(rawTemperature) * 0.25F;
    if (celsius < kMinimumTemperatureCelsius || celsius > kMaximumTemperatureCelsius) {
        return false;
    }

    sample.celsius = celsius;
    sample.valid = true;
    return true;
}

bool Ct1780ThermocoupleReader::readScratchpad(const uint8_t* address, uint8_t* scratchpad) {
    OneWire bus(dataPin_);
    if (bus.reset() == 0) {
        return false;
    }
    bus.select(address);
    bus.write(kReadScratchpad);
    for (size_t index = 0; index < 9; ++index) {
        scratchpad[index] = bus.read();
    }
    return !allBytesEqual(scratchpad, 9) && crc8(scratchpad, 8) == scratchpad[8];
}

int Ct1780ThermocoupleReader::readConfigAddress(const uint8_t* address) {
    uint8_t scratchpad[9] = {};
    if (!readScratchpad(address, scratchpad)) {
        return -1;
    }
    return scratchpad[4] & 0x0F;
}
