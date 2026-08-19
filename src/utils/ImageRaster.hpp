#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace arc_helper {

struct RasterImage {
    std::vector<uint8_t> jpeg;
    int source_width = 0;
    int source_height = 0;
    bool resized = false;
};

// Decodes JPEG/PNG and produces an official 1920x1440 background JPEG.
// Matching 4:3 sources are stretched; other aspects are center-cropped.
std::optional<RasterImage> NormalizeBackgroundImage(std::span<const uint8_t> bytes,
                                                    std::string *error = nullptr);

} // namespace arc_helper
