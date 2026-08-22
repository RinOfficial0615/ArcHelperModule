#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "config/CustomChartConfig.h"
#include "game/GameProfile.hpp"
#include "features/Feature.hpp"

namespace arc_helper {

// Feature-level lifecycle for custom charts. Importing and asset indexing live
// in CustomChartManager; this class only decides whether the feature is
// available for the active runtime and installs its hooks.
class CustomCharts : public Feature {
public:
    static CustomCharts &Instance();

    void Install(const cfg::GameProfile &profile);
    bool Enabled() const;

private:
    CustomCharts();

    static bool NonEmpty(const std::string &value) { return !value.empty(); }

    bool hooks_installed_ = false;

    AH_CFG(enabled, true);
    AH_CFG(fallback_song_id, "chart",
           [](std::string_view value) { return !value.empty(); });
    AH_CFG(rating_plus_minimum_rating, 7, 0, 20);
    AH_CFG(rating_plus_threshold, 0.69999, 0.0, 1.0);

    AH_CFG_SECTION(defaults, artist, "Unknown", NonEmpty);
    AH_CFG_SECTION(defaults, designer, "Unknown", NonEmpty);
    AH_CFG_SECTION(defaults, bpm, 120.0, cfg::custom_charts::kMinimumBpm,
                   cfg::custom_charts::kMaximumBpm);
    AH_CFG_SECTION(defaults, side, 1, cfg::custom_charts::kMinimumSide,
                   cfg::custom_charts::kMaximumSide);
    AH_CFG_SECTION(defaults, background, cfg::custom_charts::kConflictBackground,
                   NonEmpty);
    AH_CFG_SECTION(defaults, preview_start_ms, int64_t{0}, int64_t{0},
                   cfg::custom_charts::kMaximumPreviewEndMs -
                       cfg::custom_charts::kDefaultPreviewDurationMs);
    AH_CFG_SECTION(defaults, preview_duration_ms,
                   cfg::custom_charts::kDefaultPreviewDurationMs,
                   [this](const int64_t &value) {
                       return value > 0 &&
                              value <= cfg::custom_charts::kMaximumPreviewEndMs -
                                           defaults_preview_start_ms_;
                   });
    AH_CFG_SECTION(defaults, chart_difficulty, 2, 0, 4);
    AH_CFG_SECTION(defaults, rating, 0, cfg::custom_charts::kMinimumRating,
                   cfg::custom_charts::kMaximumRating);

    AH_CFG_SECTION_OPT(overrides, side, int, cfg::custom_charts::kMinimumSide,
                       cfg::custom_charts::kMaximumSide);
    AH_CFG_SECTION_OPT(overrides, background, std::string, NonEmpty);
};

} // namespace arc_helper
