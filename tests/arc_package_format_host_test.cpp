#include <cassert>
#include <string>

#include "manager/custom_chart/ArcPackageFormat.hpp"

int main() {
    using namespace arc_helper;
    CustomChartSettings defaults{};
    defaults.default_bpm = 120.0;
    defaults.default_side = 1;
    defaults.default_preview_start_ms = 0;
    defaults.default_preview_duration_ms = 30000;
    defaults.default_rating = 0;
    defaults.fallback_song_id = "x";

    {
        std::string error;
        const auto index = ParseArcIndex(
            "- directory: lirile\n  identifier: 15shrend.lirile\n"
            "  settingsFile: project.arcproj\n  version: 5\n  type: level\n"
            "- directory: pack\n  identifier: p\n  settingsFile: pack.yml\n  type: pack\n",
            error);
        assert(error.empty());
        assert(index.size() == 2);
        assert(index[0].directory == "lirile");
        assert(index[0].identifier == "15shrend.lirile");
        assert(index[0].settings_file == "project.arcproj");
        assert(index[0].type == "level");
        assert(index[0].version == 5);
        assert(index[1].type == "pack");
    }

    {
        std::string error;
        const auto charts = ParseArcProject(
            "lastOpenedChartPath: 2.aff\n"
            "charts:\n"
            "- chartPath: 2.aff\n"
            "  audioPath: base.ogg\n"
            "  title: Bounded Arc\n"
            "  difficulty: Future 11+\n"
            "  charter: >-\n"
            "    Folded\n"
            "    Charter\n"
            "  baseBpm: 120junk\n"
            "  chartConstant: 9e999\n"
            "  previewStart: -5\n"
            "  previewEnd: 10\n"
            "  skin:\n"
            "    side: light\n"
            "  colors:\n"
            "    arc:\n"
            "    - '#0CD4D4D9'\n",
            defaults, error);
        assert(error.empty());
        assert(charts.size() == 1);
        assert(charts[0].title == "Bounded Arc");
        assert(charts[0].charter == "Folded Charter");
        assert(charts[0].base_bpm == 120.0);
        assert(charts[0].chart_constant < 0);
        assert(charts[0].preview_start == 0);
        assert(charts[0].side == 0);
    }

    {
        std::string error;
        const auto charts = ParseArcProject("not: valid charts", defaults, error);
        assert(charts.empty());
        assert(!error.empty());
    }
    return 0;
}
