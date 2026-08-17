#include "utils/memory/ExecUtils.hpp"

#include <array>
#include <cstring>
#include <mutex>
#include <string>

#include "utils/memory/ProcMaps.hpp"

namespace arc_helper::mem {

bool IsAddrInLibraryExec(uintptr_t addr, std::string_view soname) {
    if (!addr || soname.empty()) return false;

    // Hot path: this helper is called frequently for vcall guards.
    static std::array<MemRange, 64> s_exec_ranges{};
    static size_t s_exec_count = 0;
    static bool s_cached = false;
    static std::string s_soname;
    static std::mutex s_cache_mutex;

    std::scoped_lock lock(s_cache_mutex);

    const bool same_soname = (s_soname.size() == soname.size()) &&
                             (soname.size() == 0 || memcmp(s_soname.data(), soname.data(), soname.size()) == 0);
    if (!s_cached || !same_soname) {
        s_soname.assign(soname.data(), soname.size());
        s_exec_count = 0;
        s_cached = ProcMaps::GetLibraryExecRanges(soname, s_exec_ranges, s_exec_count);
    }

    const auto contains = [&] {
        for (size_t i = 0; i < s_exec_count; ++i) {
            if (addr >= s_exec_ranges[i].start && addr < s_exec_ranges[i].end) return true;
        }
        return false;
    };
    if (s_cached && contains()) return true;

    // A miss can mean the library was unloaded and mapped at a new base.
    // Refresh once, but never fall back to an unrelated executable mapping.
    s_exec_count = 0;
    s_cached = ProcMaps::GetLibraryExecRanges(soname, s_exec_ranges, s_exec_count);
    return s_cached && contains();
}

} // namespace arc_helper::mem
