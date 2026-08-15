#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "config/RuntimeConfig.hpp"

int main(int argc, char **argv) {
    if (argc != 2) return 2;

    const std::filesystem::path root(argv[1]);
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    auto &config = arc_helper::RuntimeConfig::Instance();
    config.SetRootDirForTesting(root.string());
    config.EnsureLoaded();

    const auto config_path = root / "config.json";
    assert(std::filesystem::is_regular_file(config_path));
    assert(std::filesystem::is_directory(root / "charts"));
    assert(std::filesystem::is_directory(root / "cache"));

    std::ifstream input(config_path, std::ios::binary);
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string text = buffer.str();
    const std::string expected =
        "{\n"
        "  \"autoplay\": true,\n"
        "  \"networkLogger\": false,\n"
        "  \"networkBlock\": true,\n"
        "  \"sslPinningBypass\": false,\n"
        "  \"customCharts\": true\n"
        "}\n";
    assert(text == expected);
    assert(config.AutoplayEnabled());
    assert(!config.NetworkLoggerEnabled());
    assert(config.NetworkBlockEnabled());
    assert(!config.DisableSslPinsEnabled());
    assert(config.CustomChartsEnabled());

    std::cout << "runtime config host test passed: " << config_path.string() << '\n';
    return 0;
}
