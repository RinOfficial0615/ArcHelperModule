#include "features/Logging.hpp"

#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include "manager/ConfigManager.hpp"

namespace arc_helper {

Logging &Logging::Instance() {
    static Logging feature;
    return feature;
}

Logging::Logging() : Feature("Logging") {
    minimum_level_ = magic_enum::enum_cast<LogLevel>(std::string_view(level_),
                                                     magic_enum::case_insensitive)
                         .value_or(kBuildDefaultLogLevel);
    auto &config = ConfigManager::Instance();
    Logger::Instance().Configure(
        config.RootDir(),
        {{logcat_enabled_, logcat_max_length_},
         {file_enabled_, file_max_length_},
         minimum_level_});
}

} // namespace arc_helper
