#include "features/NetworkBlock.hpp"

#include <cstdint>
#include <cstring>
#include <atomic>
#include <string_view>

#include "config/NetworkBlockConfig.h"
#include "utils/Log.h"

namespace arc_helper {
namespace {

std::atomic_uint64_t g_blocked_count{0};
std::atomic_bool g_logged_block_active{false};

bool EndsWithPath(const char *url, const char *path) {
    if (!url || !path) return false;
    std::string_view url_view(url);
    std::string_view path_view(path);
    if (url_view.ends_with(path_view)) return true;

    const size_t n = path_view.size();
    if (n == 0 || n >= 255) return false;

    char tmp[256];
    std::memcpy(tmp, path, n);
    tmp[n] = '/';
    tmp[n + 1] = '\0';
    return url_view.ends_with(std::string_view(tmp, n + 1));
}

bool HasPathPrefix(const char *url, const char *path_prefix) {
    if (!url || !path_prefix) return false;
    const std::string_view url_view(url);
    const std::string_view prefix(path_prefix);
    if (prefix.empty() || !url_view.starts_with(prefix)) return false;
    const char after = url_view.size() == prefix.size() ? '\0' : url_view[prefix.size()];
    return after == '\0' || after == '/';
}

bool MatchRule(const cfg::network_block::NetworkBlockRule &rule, const char *url) {
    if (!url || !rule.pattern) return false;

    switch (rule.match_type) {
    case cfg::network_block::RuleMatchType::PathPrefix:
        return HasPathPrefix(url, rule.pattern);
    case cfg::network_block::RuleMatchType::PathSuffix:
        return EndsWithPath(url, rule.pattern);
    default:
        return false;
    }
}

bool ShouldBlock(const NetworkManager::HandlerArgs &args,
                 bool ordinary_enabled,
                 bool isolation_enabled,
                 bool block_all_requests,
                 bool block_all_non_get,
                 const char **out_reason) {
    if (out_reason) *out_reason = "none";

    if (!ordinary_enabled && !isolation_enabled) return false;
    if (ordinary_enabled && block_all_requests) {
        if (out_reason) *out_reason = "all";
        return true;
    }
    if (ordinary_enabled && block_all_non_get && args.request_type != 0) {
        if (out_reason) *out_reason = "non-get";
        return true;
    }
    if (args.url_truncated || args.url_path[0] == '\0') {
        return false;
    }

    const uint8_t method = network::HttpMethodBit(args.request_type);
    if (method == 0) return false;

    for (const auto &rule : cfg::network_block::kBlockRules) {
        if ((rule.method_mask & method) == 0) continue;
        if (!MatchRule(rule, args.url_path)) continue;
        if (out_reason) *out_reason = rule.reason ? rule.reason : "rule";
        return true;
    }

    return false;
}

} // namespace

NetworkBlock &NetworkBlock::Instance() {
    static NetworkBlock feature;
    return feature;
}

NetworkBlock::NetworkBlock() : Feature("NetworkBlock") {}

void NetworkBlock::Install(const cfg::GameProfile &profile, bool force_isolation) {
    (void)profile;
    isolation_enabled_ = force_isolation;
    if (installed_) return;
    if (!enabled_ && !isolation_enabled_) {
        installed_ = true;
        return;
    }
    const bool ok = NetworkManager::Instance().RegisterHandler(
        "NetworkBlock", cfg::network_block::kHandlerPriorityNetworkBlock, HandleNetworkRequest);

    installed_ = ok;
    ARC_LOGI("NetworkBlock: handler registration %s", ok ? "OK" : "FAILED");
}

bool NetworkBlock::HandleNetworkRequest(NetworkManager::HandlerArgs &args) {
    if (args.phase != NetworkManager::Phase::BeforeRequest) return false;

    auto &feature = Instance();
    const char *reason = "none";
    if (!ShouldBlock(args, feature.enabled_, feature.isolation_enabled_,
                     feature.block_all_requests_, feature.block_all_non_get_, &reason)) {
        return false;
    }

    args.blocked = true;
    if (reason) {
        const size_t n = std::strlen(reason);
        const size_t max_n = sizeof(args.block_reason) - 1;
        const size_t copy_n = (n < max_n) ? n : max_n;
        std::memcpy(args.block_reason, reason, copy_n);
        args.block_reason[copy_n] = '\0';
    }

    const uint64_t blocked_count = g_blocked_count.fetch_add(1, std::memory_order_relaxed) + 1;
    bool expected = false;
    if (g_logged_block_active.compare_exchange_strong(expected, true,
                                                       std::memory_order_relaxed,
                                                       std::memory_order_relaxed)) {
        ARC_LOGI("NetworkBlock: blocking enabled");
    }

    if (feature.blocked_log_limit_ == 0 || blocked_count <= feature.blocked_log_limit_) {
        ARC_LOGI("NetworkBlock: BLOCK #%llu %s %s (reason=%s)",
                 static_cast<unsigned long long>(blocked_count),
                 args.MethodStr(),
                 args.url[0] ? args.url : "(null)",
                 args.block_reason[0] ? args.block_reason : "blocked");
    }

    return true;
}

} // namespace arc_helper
