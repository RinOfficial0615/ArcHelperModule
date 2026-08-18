#include "features/AssetVirtualizer.hpp"

namespace arc_helper {

AssetVirtualizer &AssetVirtualizer::Instance() {
    static AssetVirtualizer instance;
    return instance;
}

bool AssetVirtualizer::Install(const cfg::GameProfile &profile) {
    lib_base_ = 0;
    offsets_ = profile.custom_charts;
    installed_ = true;
    return installed_;
}

} // namespace arc_helper
