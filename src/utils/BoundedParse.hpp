#pragma once

// Bounded numeric parsing for untrusted text: full-token conversion with
// explicit range validation (AGENTS.md: bounded parsing plus explicit
// validation). Shared by the custom-chart import pipeline.

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <string>
#include <string_view>

namespace arc_helper {

inline std::string TrimWhitespace(std::string_view value) {
    size_t b = 0, e = value.size();
    while (b < e && (value[b] == ' ' || value[b] == '\t' || value[b] == '\r' ||
                     value[b] == '\n')) {
        ++b;
    }
    while (e > b && (value[e - 1] == ' ' || value[e - 1] == '\t' || value[e - 1] == '\r' ||
                     value[e - 1] == '\n')) {
        --e;
    }
    return std::string(value.substr(b, e - b));
}

inline bool ParseBoundedDouble(std::string_view text, double minimum, double maximum, double &out) {
    const std::string token = TrimWhitespace(text);
    if (token.empty()) return false;
    char *end = nullptr;
    errno = 0;
    const double value = std::strtod(token.c_str(), &end);
    if (errno == ERANGE || !end || end != token.c_str() + token.size() ||
        !std::isfinite(value) || value < minimum || value > maximum) {
        return false;
    }
    out = value;
    return true;
}

inline bool ParseBoundedInt64(std::string_view text, int64_t minimum, int64_t maximum, int64_t &out) {
    const std::string token = TrimWhitespace(text);
    if (token.empty()) return false;
    char *end = nullptr;
    errno = 0;
    const long long value = std::strtoll(token.c_str(), &end, 10);
    if (errno == ERANGE || !end || end != token.c_str() + token.size() ||
        value < minimum || value > maximum) {
        return false;
    }
    out = static_cast<int64_t>(value);
    return true;
}

} // namespace arc_helper
