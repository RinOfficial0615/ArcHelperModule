#pragma once

#include <mutex>
#include <string>

namespace arc_helper {

class RuntimeConfig {
public:
    static RuntimeConfig &Instance();

    void SetPackageName(const char *package_name);
    void SetRootDir(const std::string &root_dir);
    void EnsureLoaded();

    bool AutoplayEnabled() const { return autoplay_enabled_; }
    bool NetworkLoggerEnabled() const { return network_logger_enabled_; }
    bool NetworkBlockEnabled() const { return network_block_enabled_; }
    bool DisableSslPinsEnabled() const { return disable_ssl_pins_enabled_; }
    bool CustomChartsEnabled() const { return custom_charts_enabled_; }

    const std::string &PackageName() const { return package_name_; }
    const std::string &RootDir() const { return root_dir_; }
    const std::string &ChartsDir() const { return charts_dir_; }
    const std::string &CacheDir() const { return cache_dir_; }

#ifdef ARC_HELPER_HOST_TEST
    void SetRootDirForTesting(const std::string &root_dir);
#endif

private:
    RuntimeConfig() = default;

    bool LoadLocked();

    mutable std::mutex mutex_{};
    std::string package_name_{};
    std::string root_dir_{};
    std::string charts_dir_{};
    std::string cache_dir_{};
    bool loaded_ = false;

    bool autoplay_enabled_ = true;
    bool network_logger_enabled_ = false;
    bool network_block_enabled_ = true;
    bool disable_ssl_pins_enabled_ = false;
    bool custom_charts_enabled_ = true;
};

} // namespace arc_helper
