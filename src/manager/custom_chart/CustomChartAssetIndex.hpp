#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "manager/custom_chart/CustomChartTypes.hpp"

namespace arc_helper {

// Immutable after ImportSnapshot publication.  The index owns the logical
// game paths and the extracted/official source each path resolves to.
class CustomChartAssetIndex {
public:
    bool RegisterSong(const ImportedSong &song);

    size_t Size() const { return assets_.size(); }
    const std::string *Resolve(std::string_view game_path) const;
    std::vector<std::string> ListDirectory(std::string_view game_path) const;
    bool IsCustomChartPath(std::string_view game_path, std::string *song_id = nullptr) const;

#ifdef ARC_HELPER_HOST_TEST
    bool HasPrefix(std::string_view prefix) const;
    bool HasSuffix(std::string_view suffix) const;
    bool HasValueContaining(std::string_view value) const;
#endif

private:
    std::unordered_map<std::string, std::string> assets_{};
};

} // namespace arc_helper
