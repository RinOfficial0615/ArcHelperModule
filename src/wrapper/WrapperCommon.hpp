#pragma once

#include <cstdint>

#include "utils/Log.h"
#include "config/ModuleConfig.h"
#include "manager/ConfigManager.hpp"
#include "manager/FeatureManager.hpp"
#include "manager/GameManager.hpp"
#include "manager/GameVersionManager.hpp"

namespace arc_helper::wrapper {

inline uintptr_t FindGameLibraryBase() {
    return GameManager::Instance().GetOrFindGameLibBase();
}

inline void InitResolvedFeatures() {
    if (const auto *profile = GameVersionManager::Instance().GetActiveProfile()) {
        FeatureManager::Instance().InstallAll(*profile);
    }
}

inline void PrepareFeatures() {
    ConfigManager::Instance().Load();
    FeatureManager::Instance().CreateAll();
    if (!ConfigManager::Instance().Save()) {
        ARC_LOGE("ConfigManager: normalized config was not saved");
    }
}

inline void InitFeatures() {
    PrepareFeatures();
    auto &version_manager = GameVersionManager::Instance();
    version_manager.SetResolvedCallback(&InitResolvedFeatures);
    version_manager.EnsureInstalled();
}

} // namespace arc_helper::wrapper
