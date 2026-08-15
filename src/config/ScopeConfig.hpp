#pragma once

#include <array>
#include <cctype>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <unistd.h>

namespace arc_helper::cfg::scope {

inline constexpr std::array<const char *, 3> kDefaultPackages = {
    "moe.inf.arc", "moe.low.arc", "moe.low.mes"};

inline std::string Trim(std::string_view value) {
    size_t begin = 0, end = value.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(value[begin]))) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) --end;
    return std::string(value.substr(begin, end - begin));
}

inline bool Contains(std::string_view text, std::string_view package_name) {
    size_t line_begin = 0;
    while (line_begin <= text.size()) {
        size_t line_end = text.find('\n', line_begin);
        if (line_end == std::string_view::npos) line_end = text.size();
        std::string line = Trim(text.substr(line_begin, line_end - line_begin));
        if (const size_t comment = line.find('#'); comment != std::string::npos) {
            line.resize(comment);
            line = Trim(line);
        }
        if (!line.empty() && line == package_name) return true;
        if (line_end == text.size()) break;
        line_begin = line_end + 1;
    }
    return false;
}

inline bool IsTargetPackage(int module_dir_fd, const char *package_name) {
    if (module_dir_fd < 0 || !package_name || package_name[0] == '\0') return false;
    char buffer[16 * 1024];
    const int fd = openat(module_dir_fd, "scope.txt", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        for (const char *default_package : kDefaultPackages)
            if (std::string_view(package_name) == default_package) return true;
        return false;
    }
    const ssize_t size = read(fd, buffer, sizeof(buffer));
    close(fd);
    if (size < 0) {
        for (const char *default_package : kDefaultPackages)
            if (std::string_view(package_name) == default_package) return true;
        return false;
    }
    return Contains(std::string_view(buffer, static_cast<size_t>(size)), package_name);
}

} // namespace arc_helper::cfg::scope
