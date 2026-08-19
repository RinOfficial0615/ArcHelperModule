#pragma once

#include <expected>
#include <string>
#include <vector>

#include "manager/custom_chart/AffNormalizer.hpp"
#include "manager/custom_chart/CustomChartSnapshot.hpp"

namespace arc_helper {

class CustomChartImporter {
public:
    explicit CustomChartImporter(CustomChartSettings settings);

    std::expected<ImportSnapshot, std::string> Import();

private:
    bool ImportAll(std::vector<std::string> &active_hashes, std::string &error);
    bool ImportPackage(const std::string &path, const std::string &hash);
    bool ImportArcPackage(const std::string &path, const std::string &hash);
    bool ImportRawZip(const std::string &path, const std::string &hash);
    ImportedSong MakeDefaultSong() const;
    void AddDiagnostic(std::string package, std::string item,
                       std::string status, std::string detail);
    void RecordAffDiagnostics(const std::string &package,
                              const std::string &song_id,
                              const std::string &source_name,
                              const aff::Result &normalized);

    CustomChartSettings settings_{};
    std::vector<ImportedSong> songs_{};
    std::vector<ImportDiagnostic> diagnostics_{};
};

} // namespace arc_helper
