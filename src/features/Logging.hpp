#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <magic_enum/magic_enum.hpp>

#include "features/Feature.hpp"
#include "utils/Log.h"

namespace arc_helper {

class Logging final : public Feature {
public:
    static Logging &Instance();

private:
    static constexpr size_t kLogcatMaximumLength = 1024;
    static constexpr size_t kFileMaximumLength = 1024 * 1024;

    static std::string DefaultLevelName() {
        return std::string(magic_enum::enum_name(kBuildDefaultLogLevel));
    }
    static bool IsLogLevelName(const std::string &value) {
        return magic_enum::enum_cast<LogLevel>(std::string_view(value),
                                               magic_enum::case_insensitive)
            .has_value();
    }

    Logging();

    AH_CFG(level, DefaultLevelName(), IsLogLevelName);
    AH_CFG_SECTION(logcat, enabled, true);
    AH_CFG_SECTION(logcat, max_length, size_t{1024}, size_t{1}, kLogcatMaximumLength);
    AH_CFG_SECTION(file, enabled, true);
    AH_CFG_SECTION(file, max_length, size_t{0}, [](size_t value) {
        return value == 0 || value <= kFileMaximumLength;
    });

    LogLevel minimum_level_ = kBuildDefaultLogLevel;
};

} // namespace arc_helper
