#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "manager/custom_chart/CustomChartSnapshot.hpp"

namespace arc_helper {

class CustomChartManager {
public:
    static CustomChartManager &Instance();

    bool EnsureImported(const CustomChartSettings &settings);
    bool IsReady() const { return imported_; }
    bool HasSongs() const { return snapshot_.HasSongs(); }
    size_t SongCount() const { return snapshot_.SongCount(); }
    size_t AssetCount() const { return snapshot_.AssetCount(); }

    std::string MergeSonglist(std::string_view official_json, std::string &error) const;
    const std::string *ResolveAsset(std::string_view game_path) const;
    std::vector<std::string> ListAssetDirectory(std::string_view game_path) const;
    std::vector<std::string> ListSongIdsForDifficulty(int difficulty) const;
    bool IsCustomChartPath(std::string_view game_path, std::string *song_id = nullptr) const;

#ifdef ARC_HELPER_HOST_TEST
    bool ImportForTesting(const CustomChartSettings &settings) {
        imported_ = false;
        snapshot_ = {};
        return EnsureImported(settings);
    }
    size_t SongCountForTesting() const { return snapshot_.SongCount(); }
    size_t AssetCountForTesting() const { return snapshot_.AssetCount(); }
    std::string SongsJsonForTesting() const { return snapshot_.SongsJson(); }
    bool HasAssetPrefixForTesting(std::string_view prefix) const {
        return snapshot_.assets.HasPrefix(prefix);
    }
    bool HasAssetSuffixForTesting(std::string_view suffix) const {
        return snapshot_.assets.HasSuffix(suffix);
    }
    bool HasAssetValueForTesting(std::string_view value) const {
        return snapshot_.assets.HasValueContaining(value);
    }
#endif

private:
    CustomChartManager() = default;

    ImportSnapshot snapshot_{};
    bool imported_ = false;
};

} // namespace arc_helper
