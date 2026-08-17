#include "features/CustomCharts.hpp"

#include <climits>

#include "features/AssetVirtualizer.hpp"
#include "manager/ConfigManager.hpp"
#include "manager/CustomChartManager.hpp"
#include "utils/Log.h"

namespace arc_helper {

CustomCharts &CustomCharts::Instance() {
    static CustomCharts feature;
    return feature;
}

CustomCharts::CustomCharts() : Feature("CustomCharts") {}

bool CustomCharts::Enabled() const {
    return enabled_ && ConfigManager::Instance().RootAvailable();
}

void CustomCharts::Install(const cfg::GameProfile &profile) {
    if (hooks_installed_) return;

    if (!Enabled() || !profile.capabilities.custom_charts) {
        ARC_LOGI("CustomCharts: disabled or unavailable for %s", profile.version_name);
        return;
    }

    int64_t preview_duration = default_preview_duration_ms_;
    if (default_preview_start_ms_ > INT32_MAX - preview_duration) {
        preview_duration = 30000;
        if (default_preview_start_ms_ > INT32_MAX - preview_duration) {
            ARC_LOGE("CustomCharts: preview range overflow; feature disabled");
            return;
        }
    }

    auto &config = ConfigManager::Instance();
    const CustomChartSettings settings{
        .root_dir = config.RootDir(),
        .charts_dir = config.ChartsDir(),
        .cache_dir = config.CacheDir(),
        .default_artist = default_artist_,
        .default_designer = default_designer_,
        .default_bpm = default_bpm_,
        .default_side = default_side_,
        .default_background = default_background_,
        .default_preview_start_ms = default_preview_start_ms_,
        .default_preview_duration_ms = preview_duration,
        .default_chart_difficulty = default_chart_difficulty_,
        .default_rating = default_rating_,
        .fallback_song_id = fallback_song_id_,
        .rating_plus_minimum_rating = rating_plus_minimum_rating_,
        .rating_plus_threshold = rating_plus_threshold_,
    };

    auto &manager = CustomChartManager::Instance();
    if (!manager.EnsureImported(settings) || !manager.HasSongs()) {
        ARC_LOGI("CustomCharts: no importable songs");
        return;
    }

    hooks_installed_ = AssetVirtualizer::Instance().Install(profile);
    ARC_LOGI("CustomCharts: songs=%zu assets=%zu hooks=%s",
             manager.SongCount(), manager.AssetCount(),
             hooks_installed_ ? "OK" : "FAILED");
}

} // namespace arc_helper
