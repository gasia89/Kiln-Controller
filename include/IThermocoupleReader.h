#pragma once

#include <stddef.h>
#include "Temperature.h"

class IThermocoupleReader {
public:
    virtual ~IThermocoupleReader() = default;
    virtual bool begin() = 0;
    virtual size_t count() const = 0;
    virtual bool read(size_t index, TemperatureSample& sample) = 0;
};
