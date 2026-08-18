#include <cassert>
#include <iostream>
#include <string_view>

#include "config/NetworkBlockConfig.h"

int main() {
    using namespace arc_helper::cfg::network_block;

    const BlockPolicy ordinary{
        .ordinary_enabled = true,
    };
    const BlockPolicy isolation_only{
        .isolation_active = true,
    };
    const BlockPolicy off{};

    {
        const auto hit = Evaluate({.url_path = "/16/world/map/me"}, ordinary);
        assert(hit.block);
        assert(std::string_view(hit.reason) == "world/map/me");
    }
    {
        const auto hit = Evaluate({.url_path = "/offline/world/map/me/enter"}, ordinary);
        assert(hit.block);
    }
    {
        const auto miss = Evaluate({.url_path = "/world/map"}, ordinary);
        assert(!miss.block);
    }
    for (const auto &rule : kBlockRules) {
        uint32_t request_type = 0;
        if ((rule.method_mask & kMethodGet) == 0) {
            if (rule.method_mask & kMethodPost) request_type = 1;
            else if (rule.method_mask & kMethodPut) request_type = 2;
            else if (rule.method_mask & kMethodDelete) request_type = 3;
        }
        const auto hit = Evaluate(
            {.request_type = request_type, .url_path = rule.pattern}, ordinary);
        assert(hit.block);
        assert(std::string_view(hit.reason) == rule.reason);
        if ((rule.method_mask & kMethodGet) == 0) {
            const auto miss = Evaluate({.url_path = rule.pattern}, ordinary);
            assert(!miss.block);
        }
    }
    {
        const auto hit = Evaluate(
            {.request_type = 1, .url_path = "/score/song"}, ordinary);
        assert(hit.block);
        assert(std::string_view(hit.reason) == "score/song (POST)");
    }
    {
        const auto miss = Evaluate(
            {.request_type = 1, .url_path = "/score/song"}, off);
        assert(!miss.block);
    }

    {
        const auto post = Evaluate(
            {.request_type = 1, .url_path = "/anything"}, isolation_only);
        assert(post.block);
        assert(std::string_view(post.reason) == "custom-chart-isolation");
    }
    {
        const auto token = Evaluate({.url_path = "/score/token"}, isolation_only);
        assert(token.block);
        assert(std::string_view(token.reason) == "isolation:score/token");
    }
    {
        const auto download = Evaluate(
            {.url_path = "/serve/download/me/song"}, isolation_only);
        assert(download.block);
        assert(std::string_view(download.reason) == "isolation:serve/download/me/song");
    }
    {
        const auto info = Evaluate({.url_path = "/game/info"}, isolation_only);
        assert(!info.block);
    }
    {
        const auto download = Evaluate(
            {.url_path = "/serve/download/me/song"}, ordinary);
        assert(!download.block);
    }

    std::cout << "network block host tests passed\n";
    return 0;
}
