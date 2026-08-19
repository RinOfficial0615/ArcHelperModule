#pragma once

#include <optional>
#include <string>

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

    bool hooks_installed_ = false;

    AH_CFG(enabled, true);
    AH_CFG(fallback_song_id, "chart",
           [](std::string_view value) { return !value.empty(); });
    AH_CFG(rating_plus_minimum_rating, 7, 0, 20);
    AH_CFG(rating_plus_threshold, 0.69999, 0.0, 1.0);

    std::string default_artist_{};
    std::string default_designer_{};
    double default_bpm_ = 120.0;
    int default_side_ = 1;
    std::string default_background_{};
    int64_t default_preview_start_ms_ = 0;
    int64_t default_preview_duration_ms_ = cfg::custom_charts::kDefaultPreviewDurationMs;
    int default_chart_difficulty_ = 2;
    int default_rating_ = 0;
    std::optional<int> override_side_{};
    std::optional<std::string> override_background_{};
};

} // namespace arc_helper
