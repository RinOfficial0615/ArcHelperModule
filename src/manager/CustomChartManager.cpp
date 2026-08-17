#include "manager/CustomChartManager.hpp"

#include <utility>

#include "manager/custom_chart/CustomChartImporter.hpp"
#include "utils/Log.h"

namespace arc_helper {

CustomChartManager &CustomChartManager::Instance() {
    static CustomChartManager manager;
    return manager;
}

bool CustomChartManager::EnsureImported(const CustomChartSettings &settings) {
    if (imported_) return true;
    if (settings.root_dir.empty() || settings.charts_dir.empty() || settings.cache_dir.empty()) {
        ARC_LOGE("CustomCharts: importer paths unavailable");
        return false;
    }

    CustomChartImporter importer(settings);
    auto result = importer.Import();
    if (!result) {
        ARC_LOGE("CustomCharts: importer failed: %s", result.error().c_str());
        return false;
    }

    // Publish only after importer and report writer have completed.  A
    // failed retry therefore cannot expose a half-built snapshot.
    snapshot_ = std::move(*result);
    imported_ = true;
    ARC_LOGI("CustomCharts: songs=%zu assets=%zu state=READY",
             snapshot_.SongCount(), snapshot_.AssetCount());
    return true;
}

std::string CustomChartManager::MergeSonglist(std::string_view official_json,
                                              std::string &error) const {
    return snapshot_.MergeOfficialSonglist(official_json, error);
}

const std::string *CustomChartManager::ResolveAsset(std::string_view game_path) const {
    return snapshot_.assets.Resolve(game_path);
}

std::vector<std::string> CustomChartManager::ListAssetDirectory(std::string_view game_path) const {
    return snapshot_.assets.ListDirectory(game_path);
}

std::vector<std::string> CustomChartManager::ListSongIdsForDifficulty(int difficulty) const {
    std::vector<std::string> result;
    if (difficulty < 0 ||
        difficulty >= static_cast<int>(cfg::custom_charts::kDifficultyCount)) {
        return result;
    }
    for (const auto &song : snapshot_.songs) {
        if (song.has_chart[static_cast<size_t>(difficulty)]) result.push_back(song.id);
    }
    return result;
}

bool CustomChartManager::IsCustomChartPath(std::string_view game_path,
                                           std::string *song_id) const {
    return snapshot_.assets.IsCustomChartPath(game_path, song_id);
}

} // namespace arc_helper
