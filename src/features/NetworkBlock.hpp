#pragma once

#include "features/Feature.hpp"
#include "config/GameProfile.hpp"
#include "manager/NetworkManager.hpp"

namespace arc_helper {

// Registers the lowest-priority block policy handler in NetworkManager.
//
// This feature does not install hooks directly; it only decides whether a
// request should be blocked based on configured URL/method rules.
class NetworkBlock : public Feature {
public:
    static NetworkBlock &Instance();
    void Install(const cfg::GameProfile &profile, bool force_isolation);

private:
    NetworkBlock();

    static bool HandleNetworkRequest(NetworkManager::HandlerArgs &args);

    bool installed_ = false;
    bool isolation_enabled_ = false;
    AH_CFG(enabled, true);
    AH_CFG(block_all_requests, false);
    AH_CFG(block_all_non_get, false);
    AH_CFG(blocked_log_limit, uint32_t{50});
};

} // namespace arc_helper
