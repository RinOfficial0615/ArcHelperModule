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
    auto &config = ConfigManager::Instance();
    config.EnsureObject(Name(), "defaults");
    config.EnsureObject(Name(), "overrides");
    default_artist_ = config.Read(Name(), "defaults", "artist", std::string("Unknown"),
                                  [](const std::string &value) { return !value.empty(); });
    default_designer_ = config.Read(Name(), "defaults", "designer", std::string("Unknown"),
                                    [](const std::string &value) { return !value.empty(); });
    default_bpm_ = config.Read(Name(), "defaults", "bpm", 120.0, 1.0, 10000.0);
    default_side_ = config.Read(Name(), "defaults", "side", 1, 0, 2);
    default_background_ =
        config.Read(Name(), "defaults", "background", std::string("base_conflict"),
                    [](const std::string &value) { return !value.empty(); });
    default_preview_start_ms_ = config.Read(
        Name(), "defaults", "preview_start_ms", int64_t{0}, int64_t{0},
        cfg::custom_charts::kMaximumPreviewEndMs - cfg::custom_charts::kDefaultPreviewDurationMs);
    default_preview_duration_ms_ = config.Read(
        Name(), "defaults", "preview_duration_ms", cfg::custom_charts::kDefaultPreviewDurationMs,
        [this](const int64_t &value) {
            return value > 0 &&
                   value <= cfg::custom_charts::kMaximumPreviewEndMs - default_preview_start_ms_;
        });
    default_chart_difficulty_ = config.Read(Name(), "defaults", "chart_difficulty", 2, 0, 4);
    default_rating_ = config.Read(Name(), "defaults", "rating", 0, 0, 20);
    override_side_ = config.TryRead<int>(Name(), "overrides", "side", 0, 2);
    override_background_ = config.TryRead<std::string>(
        Name(), "overrides", "background",
        [](const std::string &value) { return !value.empty(); });
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

    int64_t preview_duration = default_preview_duration_ms_;
    if (default_preview_start_ms_ >
        cfg::custom_charts::kMaximumPreviewEndMs - preview_duration) {
        preview_duration = cfg::custom_charts::kDefaultPreviewDurationMs;
        if (default_preview_start_ms_ >
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
        .override_side = override_side_,
        .override_background = override_background_,
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
