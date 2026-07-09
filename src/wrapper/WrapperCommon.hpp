#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>

#include "utils/Log.h"
#include "config/ModuleConfig.h"
#include "features/Autoplay.hpp"
#include "features/NetworkLogger.hpp"
#include "features/NetworkBlock.hpp"
#include "features/SslPinningBypass.hpp"
#include "manager/GameManager.hpp"
#include "manager/GameVersionManager.hpp"

namespace arc_autoplay::wrapper {

inline uintptr_t FindGameLibraryBase() {
    return GameManager::Instance().GetOrFindGameLibBase();
}

inline void InitResolvedFeatures() {
    Autoplay::Instance();
    NetworkLogger::Instance();
    NetworkBlock::Instance();
    SslPinningBypass::Instance();
}

inline bool IsTargetPackage(const char *pkg) {
    if (!pkg) return false;
    std::string_view pkg_sv{pkg};
    return std::ranges::any_of(cfg::module::kTargetPackages,
                               [pkg_sv](const char *target) { return target && pkg_sv == target; });
}

inline void InitFeatures() {
    auto &version_manager = GameVersionManager::Instance();
    version_manager.SetResolvedCallback(&InitResolvedFeatures);
    version_manager.EnsureInstalled();
}

} // namespace arc_autoplay::wrapper
