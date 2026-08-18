#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "manager/custom_chart/CustomChartTypes.hpp"

namespace arc_helper {

// Immutable after ImportSnapshot publication.  The index owns the logical
// game paths and the extracted/official source each path resolves to.
class CustomChartAssetIndex {
public:
    bool RegisterSong(const ImportedSong &song);

    size_t Size() const { return assets_.size(); }
    bool ContainsSongId(std::string_view song_id) const;
    const std::string *Resolve(std::string_view game_path) const;
    std::vector<std::string> ListDirectory(std::string_view game_path) const;
    bool IsCustomChartPath(std::string_view game_path, std::string *song_id = nullptr) const;

#ifdef ARC_HELPER_HOST_TEST
    bool HasPrefix(std::string_view prefix) const;
    bool HasSuffix(std::string_view suffix) const;
    bool HasValueContaining(std::string_view value) const;
#endif

private:
    struct SongIdHash {
        using is_transparent = void;
        size_t operator()(std::string_view value) const noexcept {
            return std::hash<std::string_view>{}(value);
        }
        size_t operator()(const std::string &value) const noexcept {
            return std::hash<std::string_view>{}(value);
        }
    };

    std::unordered_map<std::string, std::string> assets_{};
    std::unordered_set<std::string, SongIdHash, std::equal_to<>> song_ids_{};
};

} // namespace arc_helper
