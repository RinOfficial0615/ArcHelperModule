#pragma once

#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <mutex>
#include <string>

namespace arc_helper {

enum class LogLevel { Debug, Info, Warn, Error };

#if defined(NDEBUG)
inline constexpr LogLevel kBuildDefaultLogLevel = LogLevel::Info;
#else
inline constexpr LogLevel kBuildDefaultLogLevel = LogLevel::Debug;
#endif

struct LogSinkConfig {
    bool enabled = true;
    size_t max_length = 0;
};

struct LoggerConfig {
    LogSinkConfig logcat{true, 1024};
    LogSinkConfig file{true, 0};
    LogLevel minimum_level = kBuildDefaultLogLevel;
};

namespace log_detail {

consteval const char *BaseName(const char *path) {
    const char *base = path;
    for (const char *cursor = path; cursor && *cursor; ++cursor) {
        if (*cursor == '/' || *cursor == '\\') base = cursor + 1;
    }
    return base;
}

} // namespace log_detail

class Logger {
public:
    using LogcatWriter = void (*)(int priority, const char *line);

    static Logger &Instance();

    void Configure(const std::string &root_dir, const LoggerConfig &config);
    void Log(LogLevel level, const char *file, int line, const char *format, ...)
#if defined(__GNUC__) || defined(__clang__)
        __attribute__((format(printf, 5, 6)));
#else
        ;
#endif
    void VLog(LogLevel level, const char *file, int line, const char *format, va_list args);

#ifdef ARC_HELPER_HOST_TEST
    void SetLogcatWriterForTesting(LogcatWriter writer);
    void ResetForTesting();
#endif

private:
    Logger() = default;
    ~Logger();
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void CloseFileLocked();
    bool OpenFileLocked(const std::string &root_dir);
    void RotateFilesLocked(const std::string &logs_dir);
    void WriteLogcatLocked(LogLevel level, const std::string &line);

    std::mutex mutex_{};
    LoggerConfig config_{{true, 1024}, {false, 0}, kBuildDefaultLogLevel};
    std::FILE *file_ = nullptr;
    std::string file_path_{};
    LogcatWriter test_logcat_writer_ = nullptr;
};

} // namespace arc_helper

#define ARC_LOGD(...) \
    ::arc_helper::Logger::Instance().Log(::arc_helper::LogLevel::Debug, \
                                         ::arc_helper::log_detail::BaseName(__FILE__), \
                                         __LINE__, __VA_ARGS__)
#define ARC_LOGI(...) \
    ::arc_helper::Logger::Instance().Log(::arc_helper::LogLevel::Info, \
                                         ::arc_helper::log_detail::BaseName(__FILE__), \
                                         __LINE__, __VA_ARGS__)
#define ARC_LOGW(...) \
    ::arc_helper::Logger::Instance().Log(::arc_helper::LogLevel::Warn, \
                                         ::arc_helper::log_detail::BaseName(__FILE__), \
                                         __LINE__, __VA_ARGS__)
#define ARC_LOGE(...) \
    ::arc_helper::Logger::Instance().Log(::arc_helper::LogLevel::Error, \
                                         ::arc_helper::log_detail::BaseName(__FILE__), \
                                         __LINE__, __VA_ARGS__)
