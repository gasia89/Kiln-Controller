#pragma once

#include <stddef.h>
#include <stdint.h>
#include "HeatingProfile.h"

enum class HeatingProfileRepositoryError : uint8_t {
    None,
    InvalidProfile,
    NotFound,
    StorageFull,
    StorageFailure
};

class IHeatingProfileRepository {
public:
    virtual ~IHeatingProfileRepository() = default;
    virtual HeatingProfileRepositoryError list(HeatingProfile* profiles, size_t capacity, size_t& count) = 0;
    virtual HeatingProfileRepositoryError load(uint32_t id, HeatingProfile& profile) = 0;
    virtual HeatingProfileRepositoryError save(HeatingProfile& profile, bool replaceExisting) = 0;
    virtual HeatingProfileRepositoryError remove(uint32_t id) = 0;
};
