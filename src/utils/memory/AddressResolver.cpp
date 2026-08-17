#include "utils/memory/AddressResolver.hpp"

#include <array>
#include <cstring>
#include <limits>

#include "utils/memory/ProcMaps.hpp"

namespace arc_helper::mem {
namespace {

uintptr_t FindUniqueBytesInRanges(const MemRange *ranges,
                                  size_t range_count,
                                  const uint8_t *sig,
                                  size_t sig_len,
                                  size_t align,
                                  int *out_hits) {
    if (out_hits) *out_hits = 0;
    if (!ranges || range_count == 0 || !sig || sig_len == 0) return 0;
    if (align == 0) align = 1;

    uintptr_t found = 0;
    int hits = 0;

    for (size_t r = 0; r < range_count; ++r) {
        const uintptr_t start = ranges[r].start;
        const uintptr_t end = ranges[r].end;
        if (end <= start) continue;
        if ((end - start) < sig_len) continue;

        const uintptr_t last = end - sig_len;
        for (uintptr_t p = start;;) {
            if (memcmp(reinterpret_cast<const void *>(p), sig, sig_len) == 0) {
                hits += 1;
                if (hits == 1) {
                    found = p;
                } else {
                    if (out_hits) *out_hits = hits;
                    return 0;
                }
            }
            if (last - p < align) break;
            p += align;
        }
    }

    if (out_hits) *out_hits = hits;
    return (hits == 1) ? found : 0;
}

} // namespace

uintptr_t AddressResolver::ResolveBySignature(uintptr_t hint_offset,
                                              const uint8_t *sig,
                                              size_t sig_len,
                                              std::string_view soname) const {
    if (!lib_base_ || !sig || sig_len == 0) return 0;

    if (hint_offset > std::numeric_limits<uintptr_t>::max() - lib_base_) return 0;
    const uintptr_t hint = lib_base_ + hint_offset;
    if (ProcMaps::IsReadable(hint, sig_len) &&
        memcmp(reinterpret_cast<void *>(hint), sig, sig_len) == 0) {
        return hint;
    }

    std::array<MemRange, 64> exec_ranges{};
    size_t exec_count = 0;
    if (!ProcMaps::GetLibraryExecRanges(soname, exec_ranges, exec_count)) return 0;

    return FindUniqueBytesInRanges(exec_ranges.data(), exec_count, sig, sig_len, 4, nullptr);
}

} // namespace arc_helper::mem
