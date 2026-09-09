#include "HeatingProfile.h"

#include <math.h>
#include <string.h>

namespace {
constexpr float kSecondsPerHour = 3600.0F;

bool hasText(const char* value, size_t capacity) {
    return value != nullptr && value[0] != '\0' && memchr(value, '\0', capacity) != nullptr;
}

bool isValidSegmentType(HeatingSegmentType type) {
    return type == HeatingSegmentType::Ramp || type == HeatingSegmentType::Soak || type == HeatingSegmentType::Cooling;
}
}

float HeatingSegment::durationSeconds(float startingTemperatureCelsius) const {
    if (type == HeatingSegmentType::Soak) {
        return static_cast<float>(durationMinutes) * 60.0F;
    }
    if (rateCelsiusPerHour <= 0.0F || !isfinite(rateCelsiusPerHour)) {
        return 0.0F;
    }
    return fabsf(targetCelsius - startingTemperatureCelsius) / rateCelsiusPerHour * kSecondsPerHour;
}

HeatingProfileValidationResult HeatingProfile::validate() const {
    if (schemaVersion != 1) {
        return {HeatingProfileValidationError::InvalidSegmentType, 0};
    }
    if (!hasText(name, sizeof(name))) {
        return {HeatingProfileValidationError::EmptyName, 0};
    }
    if (segmentCount == 0) {
        return {HeatingProfileValidationError::EmptyProfile, 0};
    }
    if (segmentCount > HeatingProfileLimits::kMaxSegments) {
        return {HeatingProfileValidationError::TooManySegments, HeatingProfileLimits::kMaxSegments};
    }

    float startingTemperatureCelsius = 0.0F;
    float totalDurationSecondsValue = 0.0F;
    for (size_t index = 0; index < segmentCount; ++index) {
        const HeatingSegment& segment = segments[index];
        if (!hasText(segment.name, sizeof(segment.name))) {
            return {HeatingProfileValidationError::EmptySegmentName, index};
        }
        if (!isValidSegmentType(segment.type)) {
            return {HeatingProfileValidationError::InvalidSegmentType, index};
        }
        if (!isfinite(segment.targetCelsius) || segment.targetCelsius < 0.0F || segment.targetCelsius > HeatingProfileLimits::kMaximumTemperatureCelsius) {
            return {HeatingProfileValidationError::InvalidTarget, index};
        }
        if (segment.type == HeatingSegmentType::Soak) {
            if (segment.durationMinutes == 0 || segment.durationMinutes > HeatingProfileLimits::kMaximumSoakMinutes) {
                return {HeatingProfileValidationError::InvalidSoakDuration, index};
            }
            if (index == 0 || fabsf(segment.targetCelsius - segments[index - 1].targetCelsius) > 0.01F) {
                return {HeatingProfileValidationError::SoakMustFollowSegment, index};
            }
        } else if (!isfinite(segment.rateCelsiusPerHour) || segment.rateCelsiusPerHour <= 0.0F || segment.rateCelsiusPerHour > HeatingProfileLimits::kMaximumRampRateCelsiusPerHour) {
            return {HeatingProfileValidationError::InvalidRampRate, index};
        }

        const float segmentDurationSeconds = segment.durationSeconds(startingTemperatureCelsius);
        if (!isfinite(segmentDurationSeconds) || segmentDurationSeconds <= 0.0F) {
            return {HeatingProfileValidationError::InvalidSegmentOrder, index};
        }
        totalDurationSecondsValue += segmentDurationSeconds;
        if (totalDurationSecondsValue > HeatingProfileLimits::kMaximumDurationSeconds) {
            return {HeatingProfileValidationError::ProfileTooLong, index};
        }
        startingTemperatureCelsius = segment.targetCelsius;
    }

    return {};
}

float HeatingProfile::totalDurationSeconds() const {
    float totalDurationSecondsValue = 0.0F;
    float startingTemperatureCelsius = 0.0F;
    for (size_t index = 0; index < segmentCount && index < HeatingProfileLimits::kMaxSegments; ++index) {
        totalDurationSecondsValue += segments[index].durationSeconds(startingTemperatureCelsius);
        startingTemperatureCelsius = segments[index].targetCelsius;
    }
    return totalDurationSecondsValue;
}
