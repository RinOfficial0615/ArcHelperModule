#include "features/Logging.hpp"

#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include "manager/ConfigManager.hpp"

namespace arc_helper {
namespace {

constexpr size_t kLogcatMaximumLength = 1024;
constexpr size_t kFileMaximumLength = 1024 * 1024;

constexpr std::string_view DefaultLevelName() {
    return magic_enum::enum_name(kBuildDefaultLogLevel);
}

bool IsLogLevelName(const std::string &value) {
    return magic_enum::enum_cast<LogLevel>(
               std::string_view(value), magic_enum::case_insensitive)
        .has_value();
}

} // namespace

Logging &Logging::Instance() {
    static Logging feature;
    return feature;
}

Logging::Logging() : Feature("Logging") {
    auto &config = ConfigManager::Instance();
    const std::string level_name = config.Read(
        "Logging", "level", std::string(DefaultLevelName()), IsLogLevelName);
    minimum_level_ = magic_enum::enum_cast<LogLevel>(
                         std::string_view(level_name), magic_enum::case_insensitive)
                         .value_or(kBuildDefaultLogLevel);
    logcat_.enabled = config.Read("Logging", "logcat", "enabled", true);
    logcat_.max_length = config.Read(
        "Logging", "logcat", "max_length", size_t{1024}, size_t{1},
        kLogcatMaximumLength);
    file_.enabled = config.Read("Logging", "file", "enabled", true);
    file_.max_length = config.Read(
        "Logging", "file", "max_length", size_t{0}, [](size_t value) {
            return value == 0 || value <= kFileMaximumLength;
        });
    Logger::Instance().Configure(config.RootDir(), {logcat_, file_, minimum_level_});
}

} // namespace arc_helper
