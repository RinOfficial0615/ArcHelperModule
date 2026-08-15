#include "features/AssetVirtualizer.hpp"

namespace arc_helper {

AssetVirtualizer &AssetVirtualizer::Instance() {
    static AssetVirtualizer instance;
    return instance;
}

bool AssetVirtualizer::Install(const cfg::GameProfile &) { return true; }

} // namespace arc_helper
