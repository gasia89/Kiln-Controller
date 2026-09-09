#include "Ct1780ThermocoupleReader.h"

Ct1780ThermocoupleReader::Ct1780ThermocoupleReader(uint8_t dataPin, size_t sensorCount)
    : dataPin_(dataPin), sensorCount_(sensorCount) {}

bool Ct1780ThermocoupleReader::begin() {
    pinMode(dataPin_, INPUT_PULLUP);
    return true;
}

size_t Ct1780ThermocoupleReader::count() const {
    return sensorCount_;
}

bool Ct1780ThermocoupleReader::read(size_t index, TemperatureSample& sample) {
    sample = {};
    if (index >= sensorCount_) {
        return false;
    }

    // The CT1780 command/address protocol must be implemented from its datasheet.
    // Returning invalid data keeps the heater disabled until that contract is known.
    return false;
}
