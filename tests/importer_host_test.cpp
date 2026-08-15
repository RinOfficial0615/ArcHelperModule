#include <cassert>
#include <filesystem>
#include <iostream>

#include <nlohmann/json.hpp>

#include "config/RuntimeConfig.hpp"
#include "features/CustomChartManager.hpp"

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    const size_t expected_songs = static_cast<size_t>(std::stoul(argv[2]));
    arc_helper::RuntimeConfig::Instance().SetRootDirForTesting(argv[1]);
    arc_helper::RuntimeConfig::Instance().EnsureLoaded();
    // The fixture deliberately contains malformed global JSON. Every switch
    // must stay at its documented default rather than accepting partial text.
    assert(arc_helper::RuntimeConfig::Instance().AutoplayEnabled());
    assert(!arc_helper::RuntimeConfig::Instance().NetworkLoggerEnabled());
    assert(arc_helper::RuntimeConfig::Instance().NetworkBlockEnabled());
    assert(!arc_helper::RuntimeConfig::Instance().DisableSslPinsEnabled());
    assert(arc_helper::RuntimeConfig::Instance().CustomChartsEnabled());
    auto &manager = arc_helper::CustomChartManager::Instance();
    assert(manager.ImportForTesting());
    assert(manager.SongCountForTesting() == expected_songs);
    assert(manager.AssetCountForTesting() >= expected_songs * 4);
    const std::string songs_json = manager.SongsJsonForTesting();
    const auto songs = nlohmann::json::parse(songs_json, nullptr, false);
    assert(songs.is_array());
    // ArcCreate chartConstant 5.5/9.5 must retain the integer rating and '+' marker.
    bool found_rating_5_plus = false;
    bool found_rating_9_plus = false;
    for (const auto &song : songs) {
        for (const auto &difficulty : song.at("difficulties")) {
            if (!difficulty.value("ratingPlus", false)) continue;
            found_rating_5_plus |= difficulty.value("rating", -1) == 5;
            found_rating_9_plus |= difficulty.value("rating", -1) == 9;
        }
    }
    assert(found_rating_5_plus);
    assert(found_rating_9_plus);
    // Raw ZIP 1.zip packages djmax_wagd.jpg; it must be extracted under the
    // exact 1080 background namespace used by the game.
    assert(manager.HasAssetPrefixForTesting("img/bg/1080/ahbg_"));
    // Broken/minimal raw packages have no cover and must use the real APK
    // default jackets rather than Default/ImageFile.png.
    assert(manager.HasAssetValueForTesting("@official:img/default_jacket.jpg"));
    assert(manager.HasAssetValueForTesting("@official:img/default_jacket_256.jpg"));
    const std::filesystem::path root(argv[1]);
    assert(std::filesystem::is_regular_file(root / "manifest.json"));
    assert(std::filesystem::is_regular_file(root / "import-report.json"));
    std::cout << "importer host test passed songs=" << manager.SongCountForTesting()
              << " assets=" << manager.AssetCountForTesting() << '\n';
    return 0;
}
