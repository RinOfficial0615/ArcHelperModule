#pragma once

#include <array>
#include <cstdint>

namespace arc_helper::cfg::module {

// Shared module identity.
inline constexpr const char *kLogTag = "ArcHelper";
inline constexpr const char *kRuntimeClass = "java/lang/Runtime";
inline constexpr const char *kLibName = "libcocos2dcpp.so";

// Entry bytes of Java_low_moe_AppActivity_setAppVersion, the version-probe
// hook target resolved by its exported ELF symbol.
inline constexpr std::array<uint8_t, 16> kSig_SetAppVersion = {
    0xFF, 0x43, 0x02, 0xD1,
    0xFD, 0x7B, 0x04, 0xA9,
    0xF9, 0x2B, 0x00, 0xF9,
    0xF8, 0x5F, 0x06, 0xA9,
};

} // namespace arc_helper::cfg::module
