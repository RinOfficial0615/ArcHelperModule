#pragma once

#include "config/CustomChartConfig.h"
#include "config/GameProfile.hpp"
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

    bool hooks_installed_ = false;

    AH_CFG(enabled, true);
    AH_CFG(default_artist, "Unknown",
           [](std::string_view value) { return !value.empty(); });
    AH_CFG(default_designer, "Unknown",
           [](std::string_view value) { return !value.empty(); });
    AH_CFG(default_bpm, 120.0, 1.0, 10000.0);
    AH_CFG(default_side, 1, 0, 2);
    AH_CFG(default_background, "base_conflict",
           [](std::string_view value) { return !value.empty(); });
    AH_CFG(default_preview_start_ms, int64_t{0}, int64_t{0},
           cfg::custom_charts::kMaximumPreviewEndMs -
               cfg::custom_charts::kDefaultPreviewDurationMs);
    AH_CFG(default_preview_duration_ms, cfg::custom_charts::kDefaultPreviewDurationMs,
           [this](const int64_t &value) {
               return value > 0 &&
                      value <= cfg::custom_charts::kMaximumPreviewEndMs -
                                   default_preview_start_ms_;
           });
    AH_CFG(default_chart_difficulty, 2, 0, 4);
    AH_CFG(default_rating, 0, 0, 20);
    AH_CFG(fallback_song_id, "chart",
           [](std::string_view value) { return !value.empty(); });
    AH_CFG(rating_plus_minimum_rating, 7, 0, 20);
    AH_CFG(rating_plus_threshold, 0.69999, 0.0, 1.0);
};

} // namespace arc_helper
