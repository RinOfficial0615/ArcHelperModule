#include <cassert>
#include <filesystem>
#include <iostream>

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
    const std::filesystem::path root(argv[1]);
    assert(std::filesystem::is_regular_file(root / "manifest.json"));
    assert(std::filesystem::is_regular_file(root / "import-report.json"));
    std::cout << "importer host test passed songs=" << manager.SongCountForTesting()
              << " assets=" << manager.AssetCountForTesting() << '\n';
    return 0;
}
