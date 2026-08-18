#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "game/GameStructs.hpp"

namespace arc_helper::cfg::network_block {

// ---------------------------------------------------------------------------
//  Offsets computed from `layouts::*` mirror structs (see `GameStructs.hpp`).
//  All layouts are verified identical for 6.12.11c and 6.13.2f.
// ---------------------------------------------------------------------------
constexpr GameVersionId kLayoutVer = GameVersionId::k61211c;

// Return values used by NetworkBlock hook.
inline constexpr uint32_t kCurlSetoptRetBlocked = 0xB10Cu; // dedicated non-zero marker

// Handler priorities (larger value runs earlier).
inline constexpr int kHandlerPriorityNetworkLogger = 100;
inline constexpr int kHandlerPriorityNetworkBlock = -100;

// Signatures (first 16 bytes).

inline constexpr std::array<uint8_t, 16> kSig_HttpClient_processRequest = {
    0xFF, 0x83, 0x01, 0xD1,
    0xFD, 0x7B, 0x02, 0xA9,
    0xF7, 0x1B, 0x00, 0xF9,
    0xF6, 0x57, 0x04, 0xA9,
};

inline constexpr std::array<uint8_t, 16> kSig_Curl_easy_setopt = {
    0xFF, 0x03, 0x04, 0xD1,
    0xFD, 0x7B, 0x0F, 0xA9,
    0xFD, 0xC3, 0x03, 0x91,
    0xA2, 0x0F, 0x39, 0xA9,
};

inline constexpr size_t kHttpRequest_type_u32_off        = offsetof(layouts::HttpRequest<kLayoutVer>, type);
inline constexpr size_t kHttpRequest_body_begin_ptr_off  = offsetof(layouts::HttpRequest<kLayoutVer>, bodyBegin);
inline constexpr size_t kHttpRequest_body_end_ptr_off    = offsetof(layouts::HttpRequest<kLayoutVer>, bodyEnd);

inline constexpr size_t kHttpResponse_request_ptr_off     = offsetof(layouts::HttpResponse<kLayoutVer>, request);
inline constexpr size_t kHttpResponse_succeed_u8_off      = offsetof(layouts::HttpResponse<kLayoutVer>, succeed);
inline constexpr size_t kHttpResponse_body_vec_off        = offsetof(layouts::HttpResponse<kLayoutVer>, bodyVec);
inline constexpr size_t kHttpResponse_status_code_i64_off  = offsetof(layouts::HttpResponse<kLayoutVer>, statusCode);

// libcurl option ids (for curl_easy_setopt)
inline constexpr uint32_t kCurlOpt_URL = 10002;        // CURLOPT_URL
inline constexpr uint32_t kCurlOpt_WriteData = 10001;  // CURLOPT_WRITEDATA
inline constexpr uint32_t kCurlOpt_ErrorBuffer = 10010; // CURLOPT_ERRORBUFFER
inline constexpr size_t kCurlErrorMaxLen = 255;

// URL storage remains compile-time bounded even though logging policy is dynamic.
inline constexpr size_t kUrlMaxLen = 240;
// Network payloads are copied as borrowed views before handlers run. This is a
// producer-side hard cap; the generic logger may apply a smaller sink budget.
inline constexpr size_t kNetworkBodyCaptureMax = 1024;

enum class RuleMatchType : uint8_t {
    PathPrefix,
    PathSuffix,
};

// HTTP method bits mapped from HttpRequest type:
// 0=GET, 1=POST, 2=PUT, 3=DELETE
inline constexpr uint8_t kMethodGet = 1u << 0;
inline constexpr uint8_t kMethodPost = 1u << 1;
inline constexpr uint8_t kMethodPut = 1u << 2;
inline constexpr uint8_t kMethodDelete = 1u << 3;

struct NetworkBlockRule {
    const char *reason;
    uint8_t method_mask;
    RuleMatchType match_type;
    const char *pattern;
};

// Ordinary rules. Same nine entries as master; PathPrefix is a substring
// match with a '/' or end boundary so versioned hosts such as
// `/16/world/map/me` still hit (master used strstr on the full URL).
inline constexpr std::array<NetworkBlockRule, 9> kBlockRules = {{
    {"world/map/me", kMethodGet | kMethodPost, RuleMatchType::PathPrefix, "/world/map/me"},
    {"score/token/world", kMethodGet, RuleMatchType::PathSuffix, "/score/token/world"},
    {"course/me", kMethodGet, RuleMatchType::PathSuffix, "/course/me"},
    {"score/song (POST)", kMethodPost, RuleMatchType::PathSuffix, "/score/song"},
    {"score/token", kMethodGet, RuleMatchType::PathSuffix, "/score/token"},
    {"user/me/save (POST)", kMethodPost, RuleMatchType::PathSuffix, "/user/me/save"},
    {"multiplayer/room/create (POST)", kMethodPost, RuleMatchType::PathSuffix, "/multiplayer/me/room/create"},
    {"multiplayer/matchmaking/join (POST)", kMethodPost, RuleMatchType::PathPrefix, "/multiplayer/me/matchmaking/join"},
    {"multiplayer/matchmaking/status (POST)", kMethodPost, RuleMatchType::PathPrefix, "/multiplayer/me/matchmaking/status"},
}};

// Fixed custom-chart isolation. Applied only while a mapped custom .aff
// session is active, independent of the ordinary NetworkBlock toggle.
// Mutating methods are blocked wholesale; these GET rules cover tokens,
// downloads, world/course/purchase probes, and profile refresh.
inline constexpr std::array<NetworkBlockRule, 14> kIsolationRules = {{
    {"isolation:world/map/me", kMethodGet, RuleMatchType::PathPrefix, "/world/map/me"},
    {"isolation:score/token/world", kMethodGet, RuleMatchType::PathSuffix, "/score/token/world"},
    {"isolation:score/token/course", kMethodGet, RuleMatchType::PathSuffix, "/score/token/course"},
    {"isolation:score/token", kMethodGet, RuleMatchType::PathSuffix, "/score/token"},
    {"isolation:course/me", kMethodGet, RuleMatchType::PathSuffix, "/course/me"},
    {"isolation:serve/download/me/song", kMethodGet, RuleMatchType::PathPrefix, "/serve/download/me/song"},
    {"isolation:purchase/me", kMethodGet, RuleMatchType::PathPrefix, "/purchase/me"},
    {"isolation:purchase/bundle", kMethodGet, RuleMatchType::PathPrefix, "/purchase/bundle"},
    {"isolation:finale/progress", kMethodGet, RuleMatchType::PathSuffix, "/finale/progress"},
    {"isolation:user/me", kMethodGet, RuleMatchType::PathPrefix, "/user/me"},
    {"isolation:present/me", kMethodGet, RuleMatchType::PathPrefix, "/present/me"},
    {"isolation:insight/me", kMethodGet, RuleMatchType::PathPrefix, "/insight/me"},
    {"isolation:mission/me", kMethodGet, RuleMatchType::PathPrefix, "/mission/me"},
    {"isolation:friend/me", kMethodGet, RuleMatchType::PathPrefix, "/friend/me"},
}};

struct RequestMatch {
    uint32_t request_type = 0;
    bool url_truncated = false;
    const char *url_path = "";
};

struct BlockPolicy {
    bool ordinary_enabled = false;
    bool isolation_active = false;
    bool block_all_requests = false;
    bool block_all_non_get = false;
};

struct BlockDecision {
    bool block = false;
    const char *reason = "none";
};

inline bool EndsWithPath(std::string_view url, std::string_view path) {
    if (path.empty()) return false;
    if (url.ends_with(path)) return true;
    return url.size() > path.size() && url.ends_with('/') &&
           url.substr(0, url.size() - 1).ends_with(path);
}

inline bool HasPathPrefix(std::string_view url, std::string_view prefix) {
    if (prefix.empty()) return false;
    const size_t pos = url.find(prefix);
    if (pos == std::string_view::npos) return false;
    const size_t after = pos + prefix.size();
    return after == url.size() || url[after] == '/';
}

inline bool MatchRule(const NetworkBlockRule &rule, std::string_view url) {
    if (!rule.pattern) return false;
    switch (rule.match_type) {
    case RuleMatchType::PathPrefix:
        return HasPathPrefix(url, rule.pattern);
    case RuleMatchType::PathSuffix:
        return EndsWithPath(url, rule.pattern);
    default:
        return false;
    }
}

inline uint8_t MethodBit(uint32_t request_type) {
    if (request_type > 3) return 0;
    return static_cast<uint8_t>(1u << request_type);
}

inline bool MatchAnyRule(std::span<const NetworkBlockRule> rules,
                         uint8_t method,
                         std::string_view url,
                         const char **out_reason) {
    for (const auto &rule : rules) {
        if ((rule.method_mask & method) == 0) continue;
        if (!MatchRule(rule, url)) continue;
        if (out_reason) *out_reason = rule.reason ? rule.reason : "rule";
        return true;
    }
    return false;
}

inline BlockDecision Evaluate(const RequestMatch &request, const BlockPolicy &policy) {
    BlockDecision decision{};
    if (!policy.ordinary_enabled && !policy.isolation_active) return decision;

    if (policy.ordinary_enabled && policy.block_all_requests) {
        return {.block = true, .reason = "all"};
    }
    if (policy.isolation_active && request.request_type != 0) {
        return {.block = true, .reason = "custom-chart-isolation"};
    }
    if (policy.ordinary_enabled && policy.block_all_non_get && request.request_type != 0) {
        return {.block = true, .reason = "non-get"};
    }
    if (request.url_truncated || !request.url_path || request.url_path[0] == '\0') {
        return decision;
    }

    const uint8_t method = MethodBit(request.request_type);
    if (method == 0) return decision;

    const std::string_view url(request.url_path);
    const char *reason = "none";
    if (policy.isolation_active &&
        MatchAnyRule(kIsolationRules, method, url, &reason)) {
        return {.block = true, .reason = reason};
    }
    if (policy.ordinary_enabled &&
        MatchAnyRule(kBlockRules, method, url, &reason)) {
        return {.block = true, .reason = reason};
    }
    return decision;
}

} // namespace arc_helper::cfg::network_block
