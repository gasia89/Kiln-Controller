#pragma once

#include <Preferences.h>
#include "IHeatingProfileRepository.h"

class PreferencesHeatingProfileRepository final : public IHeatingProfileRepository {
public:
    static constexpr size_t kMaximumProfiles = 8;
    static constexpr uint8_t kRecordVersion = 1;

    HeatingProfileRepositoryError list(HeatingProfile* profiles, size_t capacity, size_t& count) override;
    HeatingProfileRepositoryError load(uint32_t id, HeatingProfile& profile) override;
    HeatingProfileRepositoryError save(HeatingProfile& profile, bool replaceExisting) override;
    HeatingProfileRepositoryError remove(uint32_t id) override;

private:
    bool readSlot(Preferences& preferences, size_t slot, HeatingProfile& profile) const;
    bool writeSlot(Preferences& preferences, size_t slot, const HeatingProfile& profile);
    int findSlot(Preferences& preferences, uint32_t id) const;
    int findFreeSlot(Preferences& preferences) const;
    static void slotKey(size_t slot, char* key, size_t capacity);
};
