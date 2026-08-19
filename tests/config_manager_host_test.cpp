#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "features/Feature.hpp"
#include "manager/ConfigManager.hpp"

namespace {

class TestFeature final : public arc_helper::Feature {
public:
    TestFeature() : Feature("TestFeature") {}

    bool Enabled() const { return enabled_; }
    const std::string &Name() const { return name_; }
    uint32_t Count() const { return count_; }
    double Ratio() const { return ratio_; }

private:
    AH_CFG(enabled, true);
    AH_CFG(name, "default", [](std::string_view value) { return !value.empty(); });
    AH_CFG(count, uint32_t{7}, uint32_t{1}, uint32_t{10});
    AH_CFG(ratio, 0.5, 0.0, 1.0);
};

nlohmann::json ReadJson(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return nlohmann::json::parse(input);
}

void WriteText(const std::filesystem::path &path, std::string_view text) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output << text;
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) return 2;

    const std::filesystem::path root(argv[1]);
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    WriteText(root / "config.json", R"json({
  "TestFeature": {
    "enabled": false,
    "name": "",
    "count": 999999999999,
    "ratio": "wrong",
    "unknown": 42
  },
  "UnknownFeature": {"keep": true},
  "Logging": {
    "logcat": {"enabled": false, "max_length": 64, "keep": "yes"}
  }
})json");

    auto &config = arc_helper::ConfigManager::Instance();
    config.SetRootDirForTesting(root.string());
    assert(config.Load());

    TestFeature feature;
    const bool file_enabled = config.Read(
        "Logging", "file", "enabled", true);
    const size_t file_limit = config.Read(
        "Logging", "file", "max_length", size_t{0}, size_t{0}, size_t{65536});
    assert(config.Save());

    assert(!feature.Enabled());
    assert(feature.Name() == "default");
    assert(feature.Count() == 7);
    assert(feature.Ratio() == 0.5);
    assert(file_enabled);
    assert(file_limit == 0);
    assert(std::filesystem::is_directory(root / "charts"));
    assert(std::filesystem::is_directory(root / "cache"));

    auto normalized = ReadJson(root / "config.json");
    assert(normalized["TestFeature"]["enabled"] == false);
    assert(normalized["TestFeature"]["name"] == "default");
    assert(normalized["TestFeature"]["count"] == 7);
    assert(normalized["TestFeature"]["ratio"] == 0.5);
    assert(normalized["TestFeature"]["unknown"] == 42);
    assert(normalized["UnknownFeature"]["keep"] == true);
    assert(normalized["Logging"]["logcat"]["keep"] == "yes");
    assert(normalized["Logging"]["file"]["enabled"] == true);

    config.EnsureObject("CustomCharts", "overrides");
    assert(config.Save());
    normalized = ReadJson(root / "config.json");
    assert(normalized["CustomCharts"]["overrides"].is_object());
    assert(!config.TryRead<int>("CustomCharts", "overrides", "side", 0, 2));
    normalized["CustomCharts"]["overrides"]["side"] = 0;
    WriteText(root / "config.json", normalized.dump());
    config.ResetForTesting(root.string());
    assert(config.Load());
    const auto override_side = config.TryRead<int>("CustomCharts", "overrides", "side", 0, 2);
    assert(override_side && *override_side == 0);

    WriteText(root / "config.json", "{broken");
    config.ResetForTesting(root.string());
    assert(config.Load());
    TestFeature defaults;
    (void)config.Read("Logging", "logcat", "enabled", true);
    (void)config.Read("Logging", "logcat", "max_length", size_t{1024});
    assert(config.Save());

    const auto regenerated = ReadJson(root / "config.json");
    assert(defaults.Enabled());
    assert(defaults.Name() == "default");
    assert(regenerated["TestFeature"]["enabled"] == true);
    assert(regenerated["Logging"]["logcat"]["max_length"] == 1024);
    assert(!regenerated.contains("UnknownFeature"));
    assert(!std::filesystem::exists(root / "config.json.tmp"));

    config.ResetForTesting({});
    assert(!config.Load());
    TestFeature in_memory_defaults;
    assert(in_memory_defaults.Enabled());
    assert(!config.RootAvailable());
    assert(!config.Save());
    return 0;
}
