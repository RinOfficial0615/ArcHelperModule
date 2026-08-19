#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "manager/custom_chart/CustomChartTypes.hpp"

namespace arc_helper {

struct ArcIndexItem {
    std::string directory;
    std::string identifier;
    std::string settings_file;
    std::string type;
    int version = 0;
};

struct ArcChartSettings {
    std::string chart_path;
    std::string audio_path;
    std::string jacket_path;
    std::string background_path;
    std::string title;
    std::string composer;
    std::string charter;
    std::string illustrator;
    std::string difficulty;
    std::string bpm_text;
    double base_bpm = 0.0;
    double chart_constant = cfg::custom_charts::kDefaultChartConstant;
    int64_t preview_start = 0;
    int64_t preview_end = 0;
    int side = -1;
};

// ArcCreate index.yml / project.arcproj (YamlDotNet camelCase).
// On YAML syntax failure, returns empty and sets error.
std::vector<ArcIndexItem> ParseArcIndex(std::string_view yaml, std::string &error);
std::vector<ArcChartSettings> ParseArcProject(std::string_view yaml,
                                              const CustomChartSettings &defaults,
                                              std::string &error);

} // namespace arc_helper
