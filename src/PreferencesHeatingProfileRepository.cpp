#include "PreferencesHeatingProfileRepository.h"

#include <stdio.h>
#include <string.h>

namespace {
constexpr char kNamespace[] = "profiles";
constexpr size_t kKeyLength = 12;
}

HeatingProfileRepositoryError PreferencesHeatingProfileRepository::list(HeatingProfile* profiles, size_t capacity, size_t& count) {
    count = 0;
    if (profiles == nullptr && capacity > 0) {
        return HeatingProfileRepositoryError::StorageFailure;
    }

    Preferences preferences;
    if (!preferences.begin(kNamespace, true)) {
        return HeatingProfileRepositoryError::StorageFailure;
    }
    for (size_t slot = 0; slot < kMaximumProfiles; ++slot) {
        HeatingProfile profile;
        if (!readSlot(preferences, slot, profile)) {
            continue;
        }
        if (count < capacity) {
            profiles[count] = profile;
        }
        ++count;
    }
    preferences.end();
    if (count > capacity) {
        return HeatingProfileRepositoryError::StorageFailure;
    }
    return HeatingProfileRepositoryError::None;
}

HeatingProfileRepositoryError PreferencesHeatingProfileRepository::load(uint32_t id, HeatingProfile& profile) {
    Preferences preferences;
    if (!preferences.begin(kNamespace, true)) {
        return HeatingProfileRepositoryError::StorageFailure;
    }
    const int slot = findSlot(preferences, id);
    const bool found = slot >= 0 && readSlot(preferences, static_cast<size_t>(slot), profile);
    preferences.end();
    return found ? HeatingProfileRepositoryError::None : HeatingProfileRepositoryError::NotFound;
}

HeatingProfileRepositoryError PreferencesHeatingProfileRepository::save(HeatingProfile& profile, bool replaceExisting) {
    if (profile.validate().valid() == false) {
        return HeatingProfileRepositoryError::InvalidProfile;
    }

    Preferences preferences;
    if (!preferences.begin(kNamespace, false)) {
        return HeatingProfileRepositoryError::StorageFailure;
    }

    int slot = profile.id == 0 ? -1 : findSlot(preferences, profile.id);
    if (slot < 0 && replaceExisting) {
        preferences.end();
        return HeatingProfileRepositoryError::NotFound;
    }
    if (slot < 0) {
        slot = findFreeSlot(preferences);
    }
    if (slot < 0) {
        preferences.end();
        return HeatingProfileRepositoryError::StorageFull;
    }
    if (profile.id == 0) {
        profile.id = static_cast<uint32_t>(slot + 1);
    }
    const bool written = writeSlot(preferences, static_cast<size_t>(slot), profile);
    preferences.end();
    return written ? HeatingProfileRepositoryError::None : HeatingProfileRepositoryError::StorageFailure;
}

HeatingProfileRepositoryError PreferencesHeatingProfileRepository::remove(uint32_t id) {
    Preferences preferences;
    if (!preferences.begin(kNamespace, false)) {
        return HeatingProfileRepositoryError::StorageFailure;
    }
    const int slot = findSlot(preferences, id);
    if (slot < 0) {
        preferences.end();
        return HeatingProfileRepositoryError::NotFound;
    }
    char key[kKeyLength] = {};
    slotKey(static_cast<size_t>(slot), key, sizeof(key));
    const bool removed = preferences.remove(key);
    preferences.end();
    return removed ? HeatingProfileRepositoryError::None : HeatingProfileRepositoryError::StorageFailure;
}

bool PreferencesHeatingProfileRepository::readSlot(Preferences& preferences, size_t slot, HeatingProfile& profile) const {
    char key[kKeyLength] = {};
    slotKey(slot, key, sizeof(key));
    if (preferences.getBytesLength(key) != sizeof(HeatingProfile)) {
        return false;
    }
    if (preferences.getBytes(key, &profile, sizeof(HeatingProfile)) != sizeof(HeatingProfile)) {
        return false;
    }
    return profile.id != 0 && profile.segmentCount > 0 && profile.segmentCount <= HeatingProfileLimits::kMaxSegments && profile.name[0] != '\0';
}

bool PreferencesHeatingProfileRepository::writeSlot(Preferences& preferences, size_t slot, const HeatingProfile& profile) {
    char key[kKeyLength] = {};
    slotKey(slot, key, sizeof(key));
    char versionKey[kKeyLength] = {};
    snprintf(versionKey, sizeof(versionKey), "v%u", static_cast<unsigned>(slot));
    return preferences.putBytes(key, &profile, sizeof(HeatingProfile)) == sizeof(HeatingProfile) && preferences.putUChar(versionKey, kRecordVersion) == kRecordVersion;
}

int PreferencesHeatingProfileRepository::findSlot(Preferences& preferences, uint32_t id) const {
    for (size_t slot = 0; slot < kMaximumProfiles; ++slot) {
        HeatingProfile profile;
        if (readSlot(preferences, slot, profile) && profile.id == id) {
            return static_cast<int>(slot);
        }
    }
    return -1;
}

int PreferencesHeatingProfileRepository::findFreeSlot(Preferences& preferences) const {
    for (size_t slot = 0; slot < kMaximumProfiles; ++slot) {
        HeatingProfile profile;
        if (!readSlot(preferences, slot, profile)) {
            return static_cast<int>(slot);
        }
    }
    return -1;
}

void PreferencesHeatingProfileRepository::slotKey(size_t slot, char* key, size_t capacity) {
    snprintf(key, capacity, "profile%u", static_cast<unsigned>(slot));
}
