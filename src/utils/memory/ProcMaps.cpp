#include "utils/memory/ProcMaps.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <vector>

#include <sys/mman.h>

namespace arc_helper::mem {
namespace {

struct PermissionRange {
    uintptr_t start = 0;
    uintptr_t end = 0;
    int permissions = 0;
};

struct PermissionCache {
    std::vector<PermissionRange> ranges;
    std::chrono::steady_clock::time_point refreshed{};
    uint64_t generation = 0;
    bool valid = false;
};

// Runtime reads are hot in gameplay hooks. Reuse one map view for at most one
// frame, while RuntimeMemory::Protect invalidates every thread immediately.
constexpr auto kPermissionCacheLifetime = std::chrono::milliseconds(16);
std::atomic_uint64_t g_permission_cache_generation{1};
thread_local PermissionCache t_permission_cache{};

bool PathMatchesSoname(std::string_view path, std::string_view soname) {
    constexpr std::string_view kDeletedSuffix = " (deleted)";
    if (path.ends_with(kDeletedSuffix)) path.remove_suffix(kDeletedSuffix.size());
    const size_t slash = path.find_last_of('/');
    const std::string_view basename = slash == std::string_view::npos
                                          ? path
                                          : path.substr(slash + 1);
    return basename == soname;
}

std::string_view TrimSpaces(std::string_view sv) {
    while (!sv.empty()) {
        const char c = sv.front();
        if (c == ' ' || c == '\t') {
            sv.remove_prefix(1);
            continue;
        }
        break;
    }

    while (!sv.empty()) {
        const char c = sv.back();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            sv.remove_suffix(1);
            continue;
        }
        break;
    }

    return sv;
}

bool ParseProcMapsLine(const char *line,
                      uintptr_t *out_start,
                      uintptr_t *out_end,
                      uintptr_t *out_off,
                      int *out_perms,
                      std::string_view *out_path) {
    if (!line || !out_start || !out_end || !out_off || !out_perms || !out_path) return false;

    uintptr_t start = 0;
    uintptr_t end = 0;
    uintptr_t off = 0;
    unsigned long inode = 0;
    unsigned int dev_major = 0;
    unsigned int dev_minor = 0;
    char perm[5] = {'\0'};
    int path_off = 0;

    const int scanned = sscanf(line,
                               "%" PRIxPTR "-%" PRIxPTR " %4s %" PRIxPTR " %x:%x %lu %n",
                               &start,
                               &end,
                               perm,
                               &off,
                               &dev_major,
                               &dev_minor,
                               &inode,
                               &path_off);
    if (scanned != 7) return false;

    int perms = 0;
    if (perm[0] == 'r') perms |= PROT_READ;
    if (perm[1] == 'w') perms |= PROT_WRITE;
    if (perm[2] == 'x') perms |= PROT_EXEC;

    std::string_view path;
    if (path_off > 0) {
        path = std::string_view(line + path_off);
        path = TrimSpaces(path);
    }

    *out_start = start;
    *out_end = end;
    *out_off = off;
    *out_perms = perms;
    *out_path = path;
    return true;
}

bool RefreshPermissionCache(PermissionCache &cache, uint64_t generation) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) {
        cache.valid = false;
        cache.ranges.clear();
        return false;
    }

    cache.ranges.clear();
    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start = 0;
        uintptr_t end = 0;
        uintptr_t off = 0;
        int permissions = 0;
        std::string_view path;
        if (!ParseProcMapsLine(line, &start, &end, &off, &permissions, &path)) continue;
        (void)off;
        (void)path;
        if (end > start) cache.ranges.push_back({start, end, permissions});
    }
    const bool read_ok = ferror(fp) == 0;
    fclose(fp);
    if (!read_ok) {
        cache.valid = false;
        cache.ranges.clear();
        return false;
    }
    cache.generation = generation;
    cache.refreshed = std::chrono::steady_clock::now();
    cache.valid = true;
    return true;
}

const PermissionCache *CurrentPermissionCache() {
    const uint64_t generation = g_permission_cache_generation.load(std::memory_order_acquire);
    const auto now = std::chrono::steady_clock::now();
    if (!t_permission_cache.valid || t_permission_cache.generation != generation ||
        now - t_permission_cache.refreshed >= kPermissionCacheLifetime) {
        if (!RefreshPermissionCache(t_permission_cache, generation)) return nullptr;
    }
    return &t_permission_cache;
}

bool CachedRangePermitted(uintptr_t addr, size_t len, int required_permissions) {
    if (len == 0 || addr == 0 || len > UINTPTR_MAX - addr) return false;
    const PermissionCache *cache = CurrentPermissionCache();
    if (!cache) return false;

    const uintptr_t end = addr + len;
    uintptr_t cursor = addr;
    for (const auto &range : cache->ranges) {
        if (range.end <= cursor) continue;
        if (range.start > cursor ||
            (range.permissions & required_permissions) != required_permissions) {
            return false;
        }
        cursor = std::min(end, range.end);
        if (cursor == end) return true;
    }
    return false;
}

} // namespace

uintptr_t ProcMaps::FindLibraryBase(std::string_view soname) {
    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;

    char line[4096];
    uintptr_t base = 0;
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start = 0;
        uintptr_t end = 0;
        uintptr_t off = 0;
        int perms = 0;
        std::string_view path;
        if (!ParseProcMapsLine(line, &start, &end, &off, &perms, &path)) continue;
        (void)end;
        (void)perms;
        if (!PathMatchesSoname(path, soname)) continue;

        if (off > start) continue;
        const uintptr_t candidate = start - off;
        if (base == 0 || candidate < base) base = candidate;
    }

    fclose(fp);
    return base;
}

bool ProcMaps::GetLibraryExecRanges(std::string_view soname,
                                    std::array<MemRange, 64> &out_ranges,
                                    size_t &out_count) {
    out_count = 0;

    FILE *fp = fopen("/proc/self/maps", "r");
    if (!fp) return false;

    char line[4096];
    while (fgets(line, sizeof(line), fp)) {
        uintptr_t start = 0;
        uintptr_t end = 0;
        uintptr_t off = 0;
        int perms = 0;
        std::string_view path;
        if (!ParseProcMapsLine(line, &start, &end, &off, &perms, &path)) continue;
        (void)off;
        if ((perms & (PROT_READ | PROT_EXEC)) != (PROT_READ | PROT_EXEC)) continue;
        if (!PathMatchesSoname(path, soname)) continue;
        if (out_count >= out_ranges.size()) break;

        out_ranges[out_count++] = MemRange{start, end};
    }

    fclose(fp);
    return out_count > 0;
}

bool ProcMaps::GetPermissions(uintptr_t addr, int &out_perms) {
    if (const PermissionCache *cache = CurrentPermissionCache()) {
        for (const auto &range : cache->ranges) {
            if (addr >= range.start && addr < range.end) {
                out_perms = range.permissions;
                return true;
            }
            if (range.start > addr) break;
        }
    }
    return false;
}

namespace {

bool IsRangePermitted(uintptr_t addr, size_t len, int required_perms) {
    return CachedRangePermitted(addr, len, required_perms);
}

} // namespace

bool ProcMaps::IsReadable(uintptr_t addr, size_t len) {
    return IsRangePermitted(addr, len, PROT_READ);
}

bool ProcMaps::IsWritable(uintptr_t addr, size_t len) {
    return IsRangePermitted(addr, len, PROT_WRITE);
}

bool ProcMaps::IsExecutable(uintptr_t addr) {
    int perms = 0;
    return GetPermissions(addr, perms) && ((perms & PROT_EXEC) != 0);
}

void ProcMaps::InvalidatePermissionCache() {
    g_permission_cache_generation.fetch_add(1, std::memory_order_acq_rel);
}

} // namespace arc_helper::mem
