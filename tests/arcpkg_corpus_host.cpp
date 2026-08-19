#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "manager/custom_chart/AffOfficialParser.hpp"
#include "manager/custom_chart/CustomChartImporter.hpp"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "usage: arcpkg_corpus_host <import-root>\n";
        return 2;
    }
    const std::string root = argv[1];
    const arc_helper::CustomChartSettings settings{
        .root_dir = root,
        .charts_dir = root + "/charts",
        .cache_dir = root + "/cache",
        .default_artist = "Unknown Artist",
        .default_designer = "Unknown Designer",
        .default_bpm = 120.0,
        .default_side = 1,
        .default_background = "base_conflict",
        .default_preview_start_ms = 0,
        .default_preview_duration_ms = 30000,
        .default_chart_difficulty = 2,
        .default_rating = 0,
        .fallback_song_id = "corpus",
        .rating_plus_minimum_rating = 7,
        .rating_plus_threshold = 0.69999,
        .override_side = std::nullopt,
        .override_background = std::nullopt,
    };
    arc_helper::CustomChartImporter importer(settings);
    const auto imported = importer.Import();
    nlohmann::json report;
    report["ok"] = imported.has_value();
    if (!imported) {
        report["error"] = imported.error();
        std::ofstream(root + "/corpus-report.json") << report.dump(2);
        std::cerr << "import failed: " << imported.error() << "\n";
        return 1;
    }

    nlohmann::json statuses = nlohmann::json::object();
    nlohmann::json dropped = nlohmann::json::array();
    for (const auto &diag : imported->diagnostics) {
        statuses[diag.status] = statuses.value(diag.status, 0) + 1;
        if (diag.status.starts_with("DROPPED") || diag.status == "SKIPPED_SONG" ||
            diag.status == "SKIPPED_PACKAGE" || diag.status == "SKIPPED_CHART" ||
            diag.status == "NON_OGG_AUDIO") {
            dropped.push_back({
                {"package", diag.package},
                {"item", diag.item},
                {"status", diag.status},
                {"detail", diag.detail},
            });
        }
    }

    int aff_ok = 0;
    int aff_fail = 0;
    nlohmann::json illegal = nlohmann::json::array();
    std::error_code ec;
    if (std::filesystem::exists(settings.cache_dir, ec)) {
        for (std::filesystem::recursive_directory_iterator it(settings.cache_dir, ec), end;
             !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file() || it->path().extension() != ".aff") continue;
            std::ifstream in(it->path(), std::ios::binary);
            const std::string text((std::istreambuf_iterator<char>(in)),
                                   std::istreambuf_iterator<char>());
            const auto check = arc_helper::aff::CheckOfficial(text);
            if (check.ok) {
                ++aff_ok;
                continue;
            }
            ++aff_fail;
            illegal.push_back({
                {"path", it->path().generic_string()},
                {"line", check.line},
                {"error", check.error},
            });
        }
    }

    report["songs"] = imported->songs.size();
    report["diagnostics"] = imported->diagnostics.size();
    report["statuses"] = statuses;
    report["notable"] = dropped;
    report["aff_ok"] = aff_ok;
    report["aff_fail"] = aff_fail;
    report["illegal"] = illegal;
    std::ofstream(root + "/corpus-report.json") << report.dump(2);
    std::cout << "songs " << imported->songs.size()
              << " aff_ok " << aff_ok
              << " aff_fail " << aff_fail
              << " diagnostics " << imported->diagnostics.size() << "\n";
    for (auto it = statuses.begin(); it != statuses.end(); ++it) {
        std::cout << "  " << it.key() << " " << it.value() << "\n";
    }
    if (aff_fail) {
        std::cerr << "normalized AFF failed official grammar\n";
        return 1;
    }
    return 0;
}
