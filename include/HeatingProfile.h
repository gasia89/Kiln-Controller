#pragma once

#include <stddef.h>
#include <stdint.h>

namespace HeatingProfileLimits {
constexpr size_t kMaxSegments = 16;
constexpr size_t kNameLength = 48;
constexpr size_t kApplicationLength = 32;
constexpr size_t kDescriptionLength = 128;
constexpr size_t kSegmentNameLength = 40;
constexpr float kMaximumTemperatureCelsius = 1372.0F;
constexpr float kMaximumRampRateCelsiusPerHour = 1000.0F;
constexpr uint32_t kMaximumSoakMinutes = 7U * 24U * 60U;
constexpr uint32_t kMaximumDurationSeconds = 30U * 24U * 60U * 60U;
}

enum class HeatingSegmentType : uint8_t {
    Ramp,
    Soak,
    Cooling
};

struct HeatingSegment {
    char name[HeatingProfileLimits::kSegmentNameLength] = {};
    HeatingSegmentType type = HeatingSegmentType::Ramp;
    float targetCelsius = 0.0F;
    float rateCelsiusPerHour = 0.0F;
    uint32_t durationMinutes = 0;

    float durationSeconds(float startingTemperatureCelsius) const;
};

enum class HeatingProfileValidationError : uint8_t {
    None,
    EmptyName,
    EmptySegmentName,
    EmptyProfile,
    TooManySegments,
    InvalidSegmentType,
    InvalidTarget,
    InvalidRampRate,
    InvalidSoakDuration,
    SoakMustFollowSegment,
    InvalidSegmentOrder,
    ProfileTooLong
};

struct HeatingProfileValidationResult {
    HeatingProfileValidationError error;
    size_t segmentIndex;

    HeatingProfileValidationResult(HeatingProfileValidationError validationError = HeatingProfileValidationError::None, size_t validationSegmentIndex = 0)
        : error(validationError), segmentIndex(validationSegmentIndex) {}

    bool valid() const { return error == HeatingProfileValidationError::None; }
};

struct HeatingProfile {
    uint32_t id = 0;
    uint16_t schemaVersion = 1;
    char name[HeatingProfileLimits::kNameLength] = {};
    char application[HeatingProfileLimits::kApplicationLength] = {};
    char description[HeatingProfileLimits::kDescriptionLength] = {};
    size_t segmentCount = 0;
    HeatingSegment segments[HeatingProfileLimits::kMaxSegments] = {};

    HeatingProfileValidationResult validate() const;
    float totalDurationSeconds() const;
};
