#pragma once

// Internal shared helper for checked [address, address+size) arithmetic.
// Not part of the public memory API.

#include <cstdint>
#include <expected>
#include <limits>

#include "utils/memory/MemoryError.hpp"

namespace arc_helper::mem::detail {

inline std::expected<uintptr_t, MemoryError> CheckedEnd(uintptr_t address, size_t size) {
    if (address == 0 || size == 0) return std::unexpected(MemoryError::InvalidRange);
    if (size > std::numeric_limits<uintptr_t>::max() - address) {
        return std::unexpected(MemoryError::Overflow);
    }
    return address + size;
}

} // namespace arc_helper::mem::detail
