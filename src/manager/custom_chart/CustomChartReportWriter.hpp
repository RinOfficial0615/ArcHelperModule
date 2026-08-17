#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "manager/custom_chart/CustomChartSnapshot.hpp"

namespace arc_helper {

class CustomChartReportWriter {
public:
    CustomChartReportWriter(std::string root_dir, std::string cache_dir);

    // Reports and orphan-cache cleanup are the last commit gate for an
    // import.  A false result means the snapshot must not be published.
    bool Publish(const ImportSnapshot &snapshot,
                 const std::vector<std::string> &active_hashes,
                 std::string &error) const;

private:
    std::string root_dir_;
    std::string cache_dir_;
};

} // namespace arc_helper
