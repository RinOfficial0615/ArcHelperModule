#include <cassert>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "utils/Log.h"

namespace {

std::vector<std::string> g_logcat_lines;

void CaptureLogcat(int, const char *line) {
    g_logcat_lines.emplace_back(line ? line : "");
}

bool IsValidUtf8(const std::string &text) {
    for (size_t i = 0; i < text.size();) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        size_t length = 1;
        if ((lead & 0x80u) == 0) length = 1;
        else if ((lead & 0xE0u) == 0xC0u) length = 2;
        else if ((lead & 0xF0u) == 0xE0u) length = 3;
        else if ((lead & 0xF8u) == 0xF0u) length = 4;
        else return false;
        if (i + length > text.size()) return false;
        for (size_t j = 1; j < length; ++j) {
            if ((static_cast<unsigned char>(text[i + j]) & 0xC0u) != 0x80u) return false;
        }
        i += length;
    }
    return true;
}

std::string ReadAll(const std::filesystem::path &path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), {});
}

} // namespace

static_assert(std::string_view(arc_helper::log_detail::BaseName("a/b/c.cpp")) == "c.cpp");
static_assert(std::string_view(arc_helper::log_detail::BaseName("a\\b\\c.cpp")) == "c.cpp");

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    const std::filesystem::path root(argv[1]);
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "logs");
    for (int i = 0; i < 5; ++i) {
        std::ofstream(root / "logs" / ("ArcHelper-20000101-00000" + std::to_string(i) + "-1.log"))
            << "old";
    }

    auto &logger = arc_helper::Logger::Instance();
    logger.ResetForTesting();
    logger.SetLogcatWriterForTesting(CaptureLogcat);
    logger.Configure(root.string(), {{true, 1024}, {true, 0}});

    const std::string long_message = std::string(1100, 'x') + "-中文内容-";
    logger.Log(arc_helper::LogLevel::Info, "sample.cpp", 12, "%s", long_message.c_str());
    assert(g_logcat_lines.size() == 1);
    assert(g_logcat_lines[0].size() == 1024);
    assert(g_logcat_lines[0].starts_with("[sample.cpp:12] "));
    assert(g_logcat_lines[0].ends_with(" [TRUNC]"));
    assert(IsValidUtf8(g_logcat_lines[0]));

    const auto files = [&] {
        std::vector<std::filesystem::path> result;
        for (const auto &entry : std::filesystem::directory_iterator(root / "logs")) {
            if (entry.path().filename().string().starts_with("ArcHelper-") &&
                entry.path().extension() == ".log") {
                result.push_back(entry.path());
            }
        }
        return result;
    }();
    assert(files.size() == 5);

    std::filesystem::path current;
    for (const auto &file : files) {
        const std::string text = ReadAll(file);
        if (text.find(long_message) != std::string::npos) current = file;
    }
    assert(!current.empty());
    const std::string file_text = ReadAll(current);
    assert(std::regex_search(
        file_text,
        std::regex(R"(^\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}\] \[INFO\] \[sample\.cpp:12\] )")));
    assert(file_text.find("[INFO] [sample.cpp:12] " + long_message) != std::string::npos);
    assert(file_text.size() > g_logcat_lines[0].size());

    const size_t before_macro = g_logcat_lines.size();
    const int macro_line = __LINE__ + 1;
    ARC_LOGW("macro location");
    assert(g_logcat_lines.size() == before_macro + 1);
    assert(g_logcat_lines.back().find(
               "[logger_host_test.cpp:" + std::to_string(macro_line) + "] macro location") !=
           std::string::npos);

    logger.Configure(root.string(), {{false, 1024}, {true, 0}});
    const size_t before_file_only = g_logcat_lines.size();
    ARC_LOGI("file only");
    assert(g_logcat_lines.size() == before_file_only);

    logger.Configure(root.string(), {{false, 1024}, {false, 0}});
    const size_t before_disabled = g_logcat_lines.size();
    ARC_LOGE("disabled sinks");
    assert(g_logcat_lines.size() == before_disabled);

    const std::filesystem::path invalid_root = root / "not-a-directory";
    std::ofstream(invalid_root) << "file";
    logger.Configure(invalid_root.string(), {{false, 1024}, {true, 0}});
    const size_t before_fallback = g_logcat_lines.size();
    ARC_LOGE("file fallback");
    assert(g_logcat_lines.size() == before_fallback + 1);
    assert(g_logcat_lines.back().find("file fallback") != std::string::npos);

    logger.ResetForTesting();
    return 0;
}
