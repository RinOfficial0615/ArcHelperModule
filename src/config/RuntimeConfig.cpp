#include "config/RuntimeConfig.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string_view>

#include "config/ModuleConfig.h"
#include "utils/Log.h"
#include "utils/MiniJson.hpp"

namespace arc_helper {
namespace {

constexpr std::string_view kDefaultConfigJson = R"json({
  "autoplay": true,
  "networkLogger": false,
  "networkBlock": true,
  "sslPinningBypass": false,
  "customCharts": true
}
)json";

bool WriteDefaultConfig(const std::string &path) {
    const std::string temporary_path = path + ".tmp";
    {
        std::ofstream defaults(temporary_path, std::ios::binary | std::ios::trunc);
        if (!defaults) return false;
        defaults.write(kDefaultConfigJson.data(),
                       static_cast<std::streamsize>(kDefaultConfigJson.size()));
        if (!defaults) return false;
    }

    std::error_code ec;
    std::filesystem::rename(temporary_path, path, ec);
    if (!ec) return true;
    std::filesystem::remove(temporary_path, ec);
    return false;
}

std::string DetectProcessPackage() {
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    std::string package;
    std::getline(cmdline, package, '\0');
    return package;
}

} // namespace

RuntimeConfig &RuntimeConfig::Instance() {
    static RuntimeConfig config;
    return config;
}

void RuntimeConfig::SetPackageName(const char *package_name) {
    if (!package_name || package_name[0] == '\0') return;
    std::scoped_lock lock(mutex_);
    if (package_name_ == package_name) return;
    package_name_ = package_name;
    root_dir_ = "/sdcard/Android/data/" + package_name_ + "/files/ArcHelper";
    charts_dir_ = root_dir_ + "/charts";
    cache_dir_ = root_dir_ + "/cache";
    loaded_ = false;
}

void RuntimeConfig::SetRootDir(const std::string &root_dir) {
    if (root_dir.empty()) return;
    std::scoped_lock lock(mutex_);
    if (root_dir_ == root_dir) return;
    root_dir_ = root_dir;
    charts_dir_ = root_dir_ + "/charts";
    cache_dir_ = root_dir_ + "/cache";
    loaded_ = false;
}

#ifdef ARC_HELPER_HOST_TEST
void RuntimeConfig::SetRootDirForTesting(const std::string &root_dir) {
    std::scoped_lock lock(mutex_);
    package_name_ = "host.test";
    root_dir_ = root_dir;
    charts_dir_ = root_dir_ + "/charts";
    cache_dir_ = root_dir_ + "/cache";
    loaded_ = false;
}
#endif

void RuntimeConfig::EnsureLoaded() {
    std::scoped_lock lock(mutex_);
    if (loaded_) return;
    loaded_ = LoadLocked();
}

bool RuntimeConfig::LoadLocked() {
    autoplay_enabled_ = cfg::module::kAutoplayEnabled;
    network_logger_enabled_ = false;
    network_block_enabled_ = cfg::module::kNetworkBlockEnabled;
    disable_ssl_pins_enabled_ = false;
    custom_charts_enabled_ = true;

    if (root_dir_.empty()) {
        package_name_ = DetectProcessPackage();
        if (!package_name_.empty()) {
            root_dir_ = "/sdcard/Android/data/" + package_name_ + "/files/ArcHelper";
            charts_dir_ = root_dir_ + "/charts";
            cache_dir_ = root_dir_ + "/cache";
        }
    }
    if (root_dir_.empty()) {
        ARC_LOGE("RuntimeConfig: package name unavailable; custom charts disabled");
        custom_charts_enabled_ = false;
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(charts_dir_, ec);
    ec.clear();
    std::filesystem::create_directories(cache_dir_, ec);

    const std::string path = root_dir_ + "/config.json";
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        const bool generated = WriteDefaultConfig(path);
        ARC_LOGI("RuntimeConfig: %s absent; beautified defaults %s at %s",
                 path.c_str(), generated ? "generated" : "could not be generated",
                 path.c_str());
        return generated;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();
    if (text.size() > 64 * 1024) {
        ARC_LOGE("RuntimeConfig: oversized %s; using defaults", path.c_str());
        return true;
    }
    const auto parsed = json::Parse(text, 8);
    if (!parsed || !parsed.value.IsObject()) {
        ARC_LOGE("RuntimeConfig: malformed %s at %zu (%s); using defaults",
                 path.c_str(), parsed.error_offset,
                 parsed.error.empty() ? "root is not an object" : parsed.error.c_str());
        return true;
    }

    auto assign = [&parsed](const char *key, bool &target) {
        const auto *value = parsed.value.Find(key);
        if (!value) return;
        if (const auto boolean = value->AsBool()) target = *boolean;
    };
    assign("autoplay", autoplay_enabled_);
    assign("networkLogger", network_logger_enabled_);
    assign("networkBlock", network_block_enabled_);
    assign("sslPinningBypass", disable_ssl_pins_enabled_);
    assign("customCharts", custom_charts_enabled_);

    ARC_LOGI("RuntimeConfig: autoplay=%d logger=%d block=%d ssl=%d custom=%d",
             autoplay_enabled_, network_logger_enabled_, network_block_enabled_,
             disable_ssl_pins_enabled_, custom_charts_enabled_);
    return true;
}

} // namespace arc_helper
