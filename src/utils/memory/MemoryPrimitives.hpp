#pragma once

#include <cstdint>
#include <expected>

#include "utils/memory/RuntimeMemory.hpp"

namespace arc_helper::mem {

// Checked variants surface the backend error; the plain helpers below trade
// that for call-site brevity and return default values on failure.
template <typename T>
inline std::expected<T, MemoryError> ReadChecked(uintptr_t addr) {
    return RuntimeMemory::Process().Read<T>(addr);
}

template <typename T>
inline std::expected<void, MemoryError> WriteChecked(uintptr_t addr, T value) {
    return RuntimeMemory::Process().Write<T>(addr, value);
}

template <typename T>
inline T Read(uintptr_t addr) {
    const auto result = RuntimeMemory::Process().Read<T>(addr);
    return result ? *result : T{};
}

template <typename T>
inline void Write(uintptr_t addr, T value) {
    (void)RuntimeMemory::Process().Write<T>(addr, value);
}

} // namespace arc_helper::mem
