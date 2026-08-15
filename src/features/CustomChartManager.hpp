#pragma once

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "config/GameProfile.hpp"

namespace arc_helper {

struct ImportedChart {
    int slot = -1;
    std::string chart_path;
    std::string source_name;
    std::string charter = "Unknown";
    std::string jacket_designer = "Unknown";
    int rating = 0;
    bool rating_plus = false;
};

struct ImportedSong {
    std::string id;
    std::string source_id;
    std::string title;
    std::string artist = "Unknown";
    std::string bpm = "120";
    double bpm_base = 120.0;
    int side = 1;
    std::string bg = "base_conflict";
    std::string bg_path;
    int64_t preview_start = 0;
    int64_t preview_end = 30000;
    std::string audio_path;
    std::string jacket_path;
    std::string jacket_256_path;
    std::array<ImportedChart, 4> charts{};
    std::array<bool, 4> has_chart{};
};

struct ImportDiagnostic {
    std::string package;
    std::string item;
    std::string status;
    std::string detail;
};

class CustomChartManager {
public:
    static CustomChartManager &Instance();

    bool EnsureInstalled(const cfg::GameProfile &profile);
    bool IsReady() const { return ready_; }
    bool HasSongs() const { return !songs_.empty(); }

    std::string MergeSonglist(std::string_view official_json, std::string &error) const;
    const std::string *ResolveAsset(std::string_view game_path) const;
    bool IsCustomChartPath(std::string_view game_path, std::string *song_id = nullptr) const;

#ifdef ARC_HELPER_HOST_TEST
    bool ImportForTesting() { return ImportAll(); }
    size_t SongCountForTesting() const { return songs_.size(); }
    size_t AssetCountForTesting() const { return assets_.size(); }
    std::string SongsJsonForTesting() const { return BuildSongsJson(); }
    bool HasAssetPrefixForTesting(std::string_view prefix) const {
        return std::any_of(assets_.begin(), assets_.end(), [prefix](const auto &item) {
            return std::string_view(item.first).starts_with(prefix);
        });
    }
    bool HasAssetValueForTesting(std::string_view value) const {
        return std::any_of(assets_.begin(), assets_.end(), [value](const auto &item) {
            return std::string_view(item.second).find(value) != std::string_view::npos;
        });
    }
#endif

private:
    CustomChartManager() = default;

    bool ImportAll();
    bool ImportPackage(const std::string &path, const std::string &hash);
    bool ImportArcPackage(const std::string &path, const std::string &hash);
    bool ImportRawZip(const std::string &path, const std::string &hash);
    void RegisterSongAssets(const ImportedSong &song);
    void WriteReports(const std::vector<std::string> &active_hashes) const;
    std::string BuildSongsJson() const;
    void AddDiagnostic(std::string package, std::string item, std::string status, std::string detail);

    std::vector<ImportedSong> songs_{};
    std::unordered_map<std::string, std::string> assets_{};
    std::vector<ImportDiagnostic> diagnostics_{};
    cfg::GameProfile profile_{};
    bool ready_ = false;
    bool hooks_installed_ = false;
};

} // namespace arc_helper
