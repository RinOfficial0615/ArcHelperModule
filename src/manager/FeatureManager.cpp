#include "manager/FeatureManager.hpp"

#include "features/Autoplay.hpp"
#include "features/CustomCharts.hpp"
#include "features/CxaThrowTracer.hpp"
#include "features/Logging.hpp"
#include "features/NetworkBlock.hpp"
#include "features/NetworkLogger.hpp"
#include "features/SslPinningBypass.hpp"
#include "manager/NetworkManager.hpp"
#include "utils/Log.h"

namespace arc_helper {

FeatureManager &FeatureManager::Instance() {
    static FeatureManager manager;
    return manager;
}

void FeatureManager::CreateAll() {
    std::scoped_lock lock(mutex_);
    CreateAllLocked();
}

void FeatureManager::CreateAllLocked() {
    if (created_) return;
    Logging::Instance();
    Autoplay::Instance();
    CxaThrowTracer::Instance();
    NetworkLogger::Instance();
    NetworkBlock::Instance();
    CustomCharts::Instance();
    SslPinningBypass::Instance();
    created_ = true;
}

void FeatureManager::InstallAll(const cfg::GameProfile &profile) {
    std::scoped_lock lock(mutex_);
    CreateAllLocked();

    CxaThrowTracer::Instance().Install(profile);
    Autoplay::Instance().Install(profile);
    NetworkLogger::Instance().Install(profile);
    const bool custom_charts_active = CustomCharts::Instance().Enabled() &&
                                      profile.capabilities.custom_charts;
    NetworkBlock::Instance().Install(profile, custom_charts_active);
    if (NetworkManager::Instance().HooksInstalled()) {
        CustomCharts::Instance().Install(profile);
    } else if (custom_charts_active) {
        ARC_LOGE("CustomCharts: network isolation unavailable; loader kept disabled");
    }
    SslPinningBypass::Instance().Install(profile);
}

} // namespace arc_helper
