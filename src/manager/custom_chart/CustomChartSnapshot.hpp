#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "manager/custom_chart/CustomChartAssetIndex.hpp"

namespace arc_helper {

// A completed import is published as one value.  Callers only receive const
// access, so hooks cannot observe a half-built song list or asset index.
struct ImportSnapshot {
    std::vector<ImportedSong> songs;
    CustomChartAssetIndex assets;
    std::vector<ImportDiagnostic> diagnostics;

    bool HasSongs() const { return !songs.empty(); }
    size_t SongCount() const { return songs.size(); }
    size_t AssetCount() const { return assets.Size(); }

    std::string SongsJson() const;
    std::string MergeOfficialSonglist(std::string_view official_json,
                                      std::string &error) const;
};

} // namespace arc_helper
