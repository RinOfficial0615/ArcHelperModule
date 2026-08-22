#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace arc_helper::mem {

struct MemRange {
    uintptr_t start;
    uintptr_t end;
};

class ProcMaps {
public:
    // Live /proc/self/maps variant. Like FindLibraryBaseFromMaps, this returns
    // the ELF load bias of the library's executable mapping, not a raw base.
    static uintptr_t FindLibraryBase(std::string_view soname);
    // ELF load bias from a /proc/<pid>/maps dump. Uses the executable PT_LOAD
    // (smallest file offset with PROT_EXEC); remapped GNU_RELRO is ignored.
    static uintptr_t FindLibraryBaseFromMaps(std::string_view maps_text,
                                             std::string_view soname);
    static bool GetLibraryExecRanges(std::string_view soname,
                                     std::array<MemRange, 64> &out_ranges,
                                     size_t &out_count);
    static bool GetPermissions(uintptr_t addr, int &out_perms);
    static bool IsReadable(uintptr_t addr, size_t len);
    static bool IsWritable(uintptr_t addr, size_t len);
    static bool IsExecutable(uintptr_t addr);
    static void InvalidatePermissionCache();
};

} // namespace arc_helper::mem
