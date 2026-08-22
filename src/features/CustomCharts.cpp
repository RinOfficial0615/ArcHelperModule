#include "features/CustomCharts.hpp"

#include "features/AssetVirtualizer.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/CustomChartManager.hpp"
#include "utils/Log.h"

namespace arc_helper {

CustomCharts &CustomCharts::Instance() {
    static CustomCharts feature;
    return feature;
}

CustomCharts::CustomCharts() : Feature("CustomCharts") {
    ConfigManager::Instance().EnsureObject(Name(), "overrides");
}

bool CustomCharts::Enabled() const {
    return enabled_ && ConfigManager::Instance().RootAvailable();
}

void CustomCharts::Install(const cfg::GameProfile &profile) {
    if (hooks_installed_) return;

    if (!Enabled() || !profile.capabilities.custom_charts) {
        ARC_LOGI("Disabled or unavailable for %s", profile.version_name);
        return;
    }

    int64_t preview_duration = defaults_preview_duration_ms_;
    if (defaults_preview_start_ms_ >
        cfg::custom_charts::kMaximumPreviewEndMs - preview_duration) {
        preview_duration = cfg::custom_charts::kDefaultPreviewDurationMs;
        if (defaults_preview_start_ms_ >
            cfg::custom_charts::kMaximumPreviewEndMs - preview_duration) {
            ARC_LOGE("Preview range overflow; feature disabled");
            return;
        }
    }

    auto &config = ConfigManager::Instance();
    const CustomChartSettings settings{
        .root_dir = config.RootDir(),
        .charts_dir = config.ChartsDir(),
        .cache_dir = config.CacheDir(),
        .default_artist = defaults_artist_,
        .default_designer = defaults_designer_,
        .default_bpm = defaults_bpm_,
        .default_side = defaults_side_,
        .default_background = defaults_background_,
        .default_preview_start_ms = defaults_preview_start_ms_,
        .default_preview_duration_ms = preview_duration,
        .default_chart_difficulty = defaults_chart_difficulty_,
        .default_rating = defaults_rating_,
        .fallback_song_id = fallback_song_id_,
        .rating_plus_minimum_rating = rating_plus_minimum_rating_,
        .rating_plus_threshold = rating_plus_threshold_,
        .override_side = overrides_side_,
        .override_background = overrides_background_,
    };

    auto &manager = CustomChartManager::Instance();
    if (!manager.EnsureImported(settings) || !manager.HasSongs()) {
        ARC_LOGI("No importable songs");
        return;
    }

    hooks_installed_ = AssetVirtualizer::Instance().Install(profile);
    ARC_LOGI("songs=%zu assets=%zu hooks=%s",
             manager.SongCount(), manager.AssetCount(),
             hooks_installed_ ? "OK" : "FAILED");
}

} // namespace arc_helper
