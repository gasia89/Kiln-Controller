#pragma once

#include <stddef.h>
#include "IThermocoupleReader.h"
#include "TimeProportionalSsr.h"

class KilnController final {
public:
    enum class Mode { Manual, Automatic };

    KilnController(IThermocoupleReader& reader, TimeProportionalSsr& coilOne, TimeProportionalSsr& coilTwo);

    void begin();
    void update(uint32_t nowMs);
    void setMode(Mode mode);
    void setTargetCelsius(float target);
    void setManualCoil(size_t coil, bool enabled, float powerPercent);
    void stop();
    Mode mode() const;
    float targetCelsius() const;
    const TemperatureSample& sample(size_t index) const;
    size_t sensorCount() const;
    const TimeProportionalSsr& coil(size_t index) const;

private:
    void applyAutomaticControl();
    void applyOutputs();

    IThermocoupleReader& reader_;
    TimeProportionalSsr* coils_[2];
    TemperatureSample samples_[4];
    bool manualEnabled_[2] = {false, false};
    float manualPower_[2] = {0.0F, 0.0F};
    Mode mode_ = Mode::Manual;
    float targetCelsius_ = 0.0F;
};
