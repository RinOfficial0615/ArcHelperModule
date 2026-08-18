#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <magic_enum/magic_enum.hpp>

#include "config/NetworkBlockConfig.h"
#include "manager/network/NetworkHandlerSnapshot.hpp"

namespace arc_helper {
class NetworkManager;
}

namespace arc_helper::network {

enum class Phase : uint8_t {
    BeforeRequest = 0,
    AfterRequest = 1,
};

enum class HttpMethod : uint32_t {
    Get = 0,
    Post,
    Put,
    Delete,
};

inline constexpr auto kHttpMethodNames = [] {
    std::array<const char *, magic_enum::enum_count<HttpMethod>()> names{};
    names[magic_enum::enum_index<HttpMethod::Get>()] = "GET";
    names[magic_enum::enum_index<HttpMethod::Post>()] = "POST";
    names[magic_enum::enum_index<HttpMethod::Put>()] = "PUT";
    names[magic_enum::enum_index<HttpMethod::Delete>()] = "DELETE";
    return names;
}();

inline const char *HttpMethodStr(uint32_t request_type) {
    const auto method = magic_enum::enum_cast<HttpMethod>(request_type);
    return method ? kHttpMethodNames[*magic_enum::enum_index(*method)] : "UNK";
}

inline uint8_t HttpMethodBit(uint32_t request_type) {
    return cfg::network_block::MethodBit(request_type);
}

enum class BufferViewStatus : uint8_t {
    Ok,
    Empty,
    NullPtr,
    Unreadable,
    InvalidVector,
    InvalidLength,
};

struct BufferView {
    const uint8_t *data = nullptr;
    size_t full_len = 0;
    size_t show_len = 0;
    BufferViewStatus status = BufferViewStatus::Empty;

    bool Truncated() const { return show_len < full_len; }

    BufferView Limit(size_t maximum) const {
        if (maximum == 0 || show_len <= maximum) return *this;
        BufferView limited = *this;
        limited.show_len = maximum;
        return limited;
    }
};

struct HandlerArgs;

// Mutable per-request context tracked by NetworkManager hook callbacks.
struct ActiveRequestCtx {
    // A context exists only while HttpClient::processRequest is executing on
    // this thread. The first tracked setopt binds its easy handle; unrelated
    // handles on the same thread are ignored instead of contaminating it.
    bool active = false;
    uintptr_t curl_handle = 0;
    uintptr_t http_client = 0;
    uintptr_t http_request = 0;
    uintptr_t http_response = 0;

    uint32_t request_type = 0xFFFFFFFFu; // 0=GET,1=POST,2=PUT,3=DELETE
    uintptr_t request_body_ptr = 0;
    size_t request_body_len = 0;

    uintptr_t response_body_vec = 0; // std::vector<char>
    int64_t response_status_code = -1;
    uint8_t response_ok = 0;

    char *curl_error_buf = nullptr;
    uint32_t sequence = 0;
    bool url_truncated = false;

    char url[cfg::network_block::kUrlMaxLen + 1] = {0};
    char url_path[cfg::network_block::kUrlMaxLen + 1] = {0};

    HandlerArgs ToHandlerArgs(Phase phase) const;
    void ApplyFromHandlerArgs(const HandlerArgs &args);
};

// Mutable args passed into handlers.
//
// Handlers may edit fields and return `true` to accept modifications.
// Returning `false` discards all edits.
struct HandlerArgs {
    Phase phase = Phase::BeforeRequest;

    uint32_t request_type = 0xFFFFFFFFu;
    size_t request_body_len = 0;
    int64_t response_status_code = -1;
    uint8_t response_ok = 0;
    uint32_t sequence = 0;
    bool url_truncated = false;

    char url[cfg::network_block::kUrlMaxLen + 1] = {0};
    char url_path[cfg::network_block::kUrlMaxLen + 1] = {0};

    bool blocked = false;
    char block_reason[96] = {0};

    // These views are populated by NetworkManager immediately before each
    // handler call. Their memory is borrowed and valid only for that dispatch.
    BufferView request_body_view{};
    BufferView response_body_view{};

private:
    friend class ::arc_helper::NetworkManager;

    uintptr_t http_client = 0;
    uintptr_t http_request = 0;
    uintptr_t http_response = 0;

    uintptr_t request_body_ptr = 0;
    uintptr_t response_body_vec = 0;
    char *curl_error_buf = nullptr;
    char curl_error[cfg::network_block::kCurlErrorMaxLen + 1] = {0};

public:
    HandlerArgs() = default;
    HandlerArgs(const ActiveRequestCtx &ctx, Phase p) : phase(p) {
        http_client = ctx.http_client;
        http_request = ctx.http_request;
        http_response = ctx.http_response;
        request_type = ctx.request_type;
        request_body_ptr = ctx.request_body_ptr;
        request_body_len = ctx.request_body_len;
        response_body_vec = ctx.response_body_vec;
        response_status_code = ctx.response_status_code;
        response_ok = ctx.response_ok;
        curl_error_buf = ctx.curl_error_buf;
        sequence = ctx.sequence;
        url_truncated = ctx.url_truncated;
        std::memcpy(url, ctx.url, sizeof(url));
        std::memcpy(url_path, ctx.url_path, sizeof(url_path));
    }

    void ApplyToContext(ActiveRequestCtx &ctx) const {
        ctx.http_client = http_client;
        ctx.http_request = http_request;
        ctx.http_response = http_response;
        ctx.request_type = request_type;
        ctx.request_body_ptr = request_body_ptr;
        ctx.request_body_len = request_body_len;
        ctx.response_body_vec = response_body_vec;
        ctx.response_status_code = response_status_code;
        ctx.response_ok = response_ok;
        ctx.curl_error_buf = curl_error_buf;
        ctx.sequence = sequence;
        ctx.url_truncated = url_truncated;
        std::memcpy(ctx.url, url, sizeof(ctx.url));
        std::memcpy(ctx.url_path, url_path, sizeof(ctx.url_path));
    }

    const char *MethodStr() const { return HttpMethodStr(request_type); }
    const char *CurlError() const { return curl_error; }
};

inline HandlerArgs ActiveRequestCtx::ToHandlerArgs(Phase phase) const {
    return HandlerArgs(*this, phase);
}

inline void ActiveRequestCtx::ApplyFromHandlerArgs(const HandlerArgs &args) {
    args.ApplyToContext(*this);
}

} // namespace arc_helper::network
