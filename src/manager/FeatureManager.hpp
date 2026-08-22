#pragma once

#include <mutex>

#include "game/GameProfile.hpp"

namespace arc_helper {

class FeatureManager {
public:
    static FeatureManager &Instance();

    void CreateAll();
    void InstallAll(const cfg::GameProfile &profile);

private:
    FeatureManager() = default;

    void CreateAllLocked();

    bool created_ = false;
    std::mutex mutex_{};
};

} // namespace arc_helper
