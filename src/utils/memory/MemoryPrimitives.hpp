#pragma once

#include <cstdint>

#include "utils/memory/RuntimeMemory.hpp"

namespace arc_helper::mem {

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
