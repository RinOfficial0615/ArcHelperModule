#include "manager/custom_chart/CustomChartAssetIndex.hpp"

#include <algorithm>
#include <cctype>
#include <set>

namespace arc_helper {
namespace {

std::string Extension(std::string_view path) {
    const size_t slash = path.find_last_of("/\\");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string_view::npos || (slash != std::string_view::npos && dot < slash)) {
        return {};
    }
    std::string result(path.substr(dot));
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

std::string Canonicalize(std::string_view game_path) {
    while (game_path.starts_with('/')) game_path.remove_prefix(1);
    std::string normalized(game_path);
    for (const auto prefix : cfg::custom_charts::kAssetPathPrefixes) {
        if (!normalized.starts_with(prefix)) continue;
        normalized.erase(0, prefix.size());
        break;
    }
    if (!normalized.starts_with(cfg::custom_charts::kSongsPrefix)) return normalized;
    const size_t id_end = normalized.find('/', cfg::custom_charts::kSongsPrefix.size());
    if (id_end == std::string::npos) return normalized;
    const std::string_view song_root(normalized.data(), id_end + 1);
    const std::string_view remainder(normalized.data() + id_end + 1,
                                     normalized.size() - id_end - 1);
    if (remainder.starts_with("1080/")) {
        return std::string(song_root) + std::string(remainder.substr(5));
    }
    if (remainder.starts_with("1080_")) {
        return std::string(song_root) + std::string(remainder.substr(5));
    }
    return normalized;
}

bool Add(std::unordered_map<std::string, std::string> &assets,
         std::string key, std::string value) {
    if (const auto it = assets.find(key); it != assets.end()) return it->second == value;
    assets.emplace(std::move(key), std::move(value));
    return true;
}

} // namespace

bool CustomChartAssetIndex::RegisterSong(const ImportedSong &song) {
    if (song.id.empty() || song_ids_.contains(song.id)) return false;
    const std::string root = std::string(cfg::custom_charts::kSongsPrefix) + song.id + "/";
    if (!Add(assets_, root + cfg::custom_charts::kAudioAssetName, song.audio_path)) return false;
    if (!Add(assets_, root + cfg::custom_charts::kJacketAssetName, song.jacket_path)) return false;
    if (!Add(assets_, root + cfg::custom_charts::kJacket256AssetName,
             song.jacket_256_path.empty() ? song.jacket_path : song.jacket_256_path)) {
        return false;
    }
    if (!song.bg_path.empty()) {
        const std::string bg_root = std::string(cfg::custom_charts::kBackgroundAssetPrefix) + song.bg;
        if (!Add(assets_, bg_root + ".jpg", song.bg_path) ||
            !Add(assets_, bg_root + ".png", song.bg_path) ||
            !Add(assets_, bg_root + "_clear.png", song.bg_path)) {
            return false;
        }
    }
    for (size_t slot = 0; slot < cfg::custom_charts::kDifficultyCount; ++slot) {
        if (song.has_chart[slot] &&
            !Add(assets_,
                 cfg::custom_charts::LocalChartAssetPath(song.id, slot),
                 song.charts[slot].chart_path)) {
            return false;
        }
    }
    song_ids_.emplace(song.id);
    return true;
}

bool CustomChartAssetIndex::ContainsSongId(std::string_view song_id) const {
    return !song_id.empty() && song_ids_.contains(song_id);
}

const std::string *CustomChartAssetIndex::Resolve(std::string_view game_path) const {
    const std::string normalized = Canonicalize(game_path);
    const auto it = assets_.find(normalized);
    return it == assets_.end() ? nullptr : &it->second;
}

std::vector<std::string> CustomChartAssetIndex::ListDirectory(std::string_view game_path) const {
    std::string directory = Canonicalize(game_path);
    while (directory.ends_with('/')) directory.pop_back();
    if (!directory.empty()) directory.push_back('/');
    std::set<std::string> entries;
    for (const auto &[logical_path, source_path] : assets_) {
        (void)source_path;
        if (!std::string_view(logical_path).starts_with(directory)) continue;
        const std::string_view remainder(logical_path.data() + directory.size(),
                                         logical_path.size() - directory.size());
        if (remainder.empty()) continue;
        const size_t slash = remainder.find('/');
        entries.emplace(remainder.substr(0, slash));
    }
    return {entries.begin(), entries.end()};
}

bool CustomChartAssetIndex::IsCustomChartPath(std::string_view game_path,
                                              std::string *song_id) const {
    const std::string normalized = Canonicalize(game_path);
    if (!normalized.starts_with(cfg::custom_charts::kSongsPrefix) ||
        Extension(normalized) != ".aff" || !Resolve(normalized)) {
        return false;
    }
    if (song_id) {
        const auto prefix = cfg::custom_charts::kSongsPrefix;
        const size_t slash = normalized.find('/', prefix.size());
        *song_id = slash == std::string_view::npos
                       ? std::string{}
                       : normalized.substr(prefix.size(), slash - prefix.size());
    }
    return true;
}

#ifdef ARC_HELPER_HOST_TEST
bool CustomChartAssetIndex::HasPrefix(std::string_view prefix) const {
    return std::any_of(assets_.begin(), assets_.end(), [prefix](const auto &item) {
        return std::string_view(item.first).starts_with(prefix);
    });
}

bool CustomChartAssetIndex::HasSuffix(std::string_view suffix) const {
    return std::any_of(assets_.begin(), assets_.end(), [suffix](const auto &item) {
        return std::string_view(item.first).ends_with(suffix);
    });
}

bool CustomChartAssetIndex::HasValueContaining(std::string_view value) const {
    return std::any_of(assets_.begin(), assets_.end(), [value](const auto &item) {
        return std::string_view(item.second).find(value) != std::string_view::npos;
    });
}
#endif

} // namespace arc_helper
