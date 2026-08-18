#pragma once

#include "features/Feature.hpp"
#include "game/GameProfile.hpp"
#include "manager/NetworkManager.hpp"

namespace arc_helper {

// Registers the lowest-priority block policy handler in NetworkManager.
//
// Ordinary rules follow config. Custom-chart isolation is a separate policy
// that only reads CustomChartGameplaySession and does not depend on `enabled`.
class NetworkBlock : public Feature {
public:
    static NetworkBlock &Instance();
    void Install(const cfg::GameProfile &profile, bool isolation_armed);

private:
    NetworkBlock();

    static bool HandleNetworkRequest(NetworkManager::HandlerArgs &args);

    bool installed_ = false;
    bool isolation_armed_ = false;
    AH_CFG(enabled, true);
    AH_CFG(block_all_requests, false);
    AH_CFG(block_all_non_get, false);
    AH_CFG(blocked_log_limit, uint32_t{50});
};

} // namespace arc_helper
