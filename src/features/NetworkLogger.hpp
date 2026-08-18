#pragma once

#include "features/Feature.hpp"
#include "config/GameProfile.hpp"
#include "manager/NetworkManager.hpp"

namespace arc_helper {

// Registers a high-priority audit handler in NetworkManager.
//
// This handler logs request/response information and never blocks traffic.
class NetworkLogger : public Feature {
public:
    static NetworkLogger &Instance();
    void Install(const cfg::GameProfile &profile);

private:
    NetworkLogger();

    static bool HandleNetworkRequest(NetworkManager::HandlerArgs &args);

    bool installed_ = false;
    AH_CFG(enabled, false);
    AH_CFG(log_all_requests, true);
    AH_CFG(log_request_body, true);
    AH_CFG(request_body_only_non_get, true);
    AH_CFG(log_response, true);
    AH_CFG(strip_query, true);
    AH_CFG(request_limit, uint32_t{200});
    AH_CFG(body_capture_limit, uint32_t{1024}, uint32_t{0},
           uint32_t{cfg::network_block::kNetworkBodyCaptureMax});
};

} // namespace arc_helper
