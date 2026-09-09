#include "KilnController.h"

KilnController::KilnController(IThermocoupleReader& reader, TimeProportionalSsr& coilOne, TimeProportionalSsr& coilTwo)
    : reader_(reader), coils_{&coilOne, &coilTwo} {}

void KilnController::begin() {
    coils_[0]->begin();
    coils_[1]->begin();
    for (auto& sample : samples_) {
        sample = {};
    }
    stop();
    if (!reader_.begin()) {
        stop();
    }
}

void KilnController::update(uint32_t nowMs) {
    for (size_t index = 0; index < reader_.count() && index < 4; ++index) {
        reader_.read(index, samples_[index]);
    }

    if (mode_ == Mode::Automatic) {
        applyAutomaticControl();
    } else if (mode_ == Mode::Profile) {
        if (!hasValidSample()) {
            profileExecutor_.stop();
        } else {
            profileExecutor_.update(nowMs);
        }
        if (profileExecutor_.state() == HeatingProfileRunState::Running) {
            applyProfileControl();
        } else {
            manualEnabled_[0] = false;
            manualEnabled_[1] = false;
            manualPower_[0] = 0.0F;
            manualPower_[1] = 0.0F;
        }
    }
    applyOutputs();
    coils_[0]->update(nowMs);
    coils_[1]->update(nowMs);
}

void KilnController::setMode(Mode mode) {
    if (mode != Mode::Profile) {
        profileExecutor_.stop();
    }
    mode_ = mode;
    if (mode_ == Mode::Automatic || mode_ == Mode::Profile) {
        manualEnabled_[0] = false;
        manualEnabled_[1] = false;
    }
}

void KilnController::setTargetCelsius(float target) {
    targetCelsius_ = max(0.0F, target);
}

void KilnController::setManualCoil(size_t coil, bool enabled, float powerPercent) {
    if (coil >= 2) return;
    manualEnabled_[coil] = enabled;
    manualPower_[coil] = constrain(powerPercent, 0.0F, 100.0F);
}

bool KilnController::startProfile(const HeatingProfile& profile, uint32_t nowMs) {
    float hottest = -10000.0F;
    bool hasReading = false;
    for (size_t index = 0; index < reader_.count() && index < 4; ++index) {
        if (samples_[index].valid) {
            hottest = max(hottest, samples_[index].celsius);
            hasReading = true;
        }
    }
    if (!hasReading || !profileExecutor_.start(profile, hottest, nowMs)) {
        return false;
    }
    mode_ = Mode::Profile;
    manualEnabled_[0] = false;
    manualEnabled_[1] = false;
    return true;
}

void KilnController::pauseProfile(uint32_t nowMs) { profileExecutor_.pause(nowMs); }
void KilnController::resumeProfile(uint32_t nowMs) { profileExecutor_.resume(nowMs); }

void KilnController::stop() {
    profileExecutor_.stop();
    manualEnabled_[0] = false;
    manualEnabled_[1] = false;
    coils_[0]->setPowerPercent(0.0F);
    coils_[1]->setPowerPercent(0.0F);
    coils_[0]->forceOff();
    coils_[1]->forceOff();
}

KilnController::Mode KilnController::mode() const { return mode_; }
float KilnController::targetCelsius() const {
    return mode_ == Mode::Profile ? profileExecutor_.targetCelsius() : targetCelsius_;
}
HeatingProfileRunState KilnController::profileState() const { return profileExecutor_.state(); }
const HeatingProfileExecutor& KilnController::profileExecutor() const { return profileExecutor_; }
size_t KilnController::sensorCount() const { return reader_.count(); }
const TemperatureSample& KilnController::sample(size_t index) const { return samples_[index]; }
const TimeProportionalSsr& KilnController::coil(size_t index) const { return *coils_[index]; }

void KilnController::applyAutomaticControl() {
    applyTargetControl(targetCelsius_);
}

void KilnController::applyProfileControl() {
    applyTargetControl(profileExecutor_.targetCelsius());
}

void KilnController::applyTargetControl(float target) {
    float hottest = -10000.0F;
    bool hasReading = false;
    for (size_t index = 0; index < reader_.count() && index < 4; ++index) {
        if (samples_[index].valid) {
            hottest = max(hottest, samples_[index].celsius);
            hasReading = true;
        }
    }

    // Conservative bang-bang baseline. Replace with a tuned PID/profile controller.
    const float power = hasReading && hottest < target ? 100.0F : 0.0F;
    manualEnabled_[0] = hasReading && power > 0.0F;
    manualEnabled_[1] = manualEnabled_[0];
    manualPower_[0] = power;
    manualPower_[1] = power;
}

bool KilnController::hasValidSample() const {
    for (size_t index = 0; index < reader_.count() && index < 4; ++index) {
        if (samples_[index].valid) {
            return true;
        }
    }
    return false;
}

void KilnController::applyOutputs() {
    for (size_t index = 0; index < 2; ++index) {
        coils_[index]->setPowerPercent(manualEnabled_[index] ? manualPower_[index] : 0.0F);
    }
}
