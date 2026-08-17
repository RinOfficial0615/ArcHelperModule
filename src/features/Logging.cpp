#include "features/Logging.hpp"

#include "manager/ConfigManager.hpp"

namespace arc_helper {
namespace {

constexpr size_t kLogcatMaximumLength = 1024;
constexpr size_t kFileMaximumLength = 1024 * 1024;

} // namespace

Logging &Logging::Instance() {
    static Logging feature;
    return feature;
}

Logging::Logging() : Feature("Logging") {
    auto &config = ConfigManager::Instance();
    logcat_.enabled = config.Read("Logging", "logcat", "enabled", true);
    logcat_.max_length = config.Read(
        "Logging", "logcat", "max_length", size_t{1024}, size_t{1},
        kLogcatMaximumLength);
    file_.enabled = config.Read("Logging", "file", "enabled", true);
    file_.max_length = config.Read(
        "Logging", "file", "max_length", size_t{0}, [](size_t value) {
            return value == 0 || value <= kFileMaximumLength;
        });
    Logger::Instance().Configure(config.RootDir(), {logcat_, file_});
}

} // namespace arc_helper
