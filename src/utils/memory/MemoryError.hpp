#pragma once

#include <cstdint>

namespace arc_helper::mem {

enum class MemoryError : uint8_t {
    InvalidState,
    InvalidRange,
    Overflow,
    Unmapped,
    PermissionDenied,
    Misaligned,
    SignatureMismatch,
    ProtectionChangeFailed,
    WriteFailed,
    RestoreFailed,
    UnsupportedInstruction,
    BackendInvalid,
};

} // namespace arc_helper::mem
