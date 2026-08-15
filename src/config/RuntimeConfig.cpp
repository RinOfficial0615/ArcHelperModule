#include "config/RuntimeConfig.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "config/ModuleConfig.h"
#include "utils/Log.h"
#include "utils/MiniJson.hpp"

namespace arc_helper {
namespace {

std::string DetectProcessPackage() {
    std::ifstream cmdline("/proc/self/cmdline", std::ios::binary);
    std::string package;
    std::getline(cmdline, package, '\0');
    for (const char *target : cfg::module::kTargetPackages) {
        if (target && package == target) return package;
    }
    return {};
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
    LoadLocked();
    loaded_ = true;
}

void RuntimeConfig::LoadLocked() {
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
        return;
    }

    std::error_code ec;
    std::filesystem::create_directories(charts_dir_, ec);
    ec.clear();
    std::filesystem::create_directories(cache_dir_, ec);

    const std::string path = root_dir_ + "/config.json";
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        ARC_LOGI("RuntimeConfig: %s absent; using defaults", path.c_str());
        std::ofstream defaults(path, std::ios::binary | std::ios::trunc);
        if (defaults) {
            defaults << "{\n"
                     << "  \"autoplay\": true,\n"
                     << "  \"networkLogger\": false,\n"
                     << "  \"networkBlock\": true,\n"
                     << "  \"sslPinningBypass\": false,\n"
                     << "  \"customCharts\": true\n"
                     << "}\n";
        }
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string text = buffer.str();
    if (text.size() > 64 * 1024) {
        ARC_LOGE("RuntimeConfig: oversized %s; using defaults", path.c_str());
        return;
    }
    const auto parsed = json::Parse(text, 8);
    if (!parsed || !parsed.value.IsObject()) {
        ARC_LOGE("RuntimeConfig: malformed %s at %zu (%s); using defaults",
                 path.c_str(), parsed.error_offset,
                 parsed.error.empty() ? "root is not an object" : parsed.error.c_str());
        return;
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
}

} // namespace arc_helper
