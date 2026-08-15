#pragma once

#include <cstdint>

#include "config/GameProfile.hpp"

namespace arc_helper {

class AssetVirtualizer {
public:
    static AssetVirtualizer &Instance();
    bool Install(const cfg::GameProfile &profile);

private:
    AssetVirtualizer() = default;
    uintptr_t lib_base_ = 0;
    cfg::CustomChartsOffsets offsets_{};
    bool installed_ = false;
};

} // namespace arc_helper
