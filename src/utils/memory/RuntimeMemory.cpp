#include "utils/memory/RuntimeMemory.hpp"

#include <cstring>
#include <limits>

#if defined(__ANDROID__) || defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "utils/memory/ProcMaps.hpp"

namespace arc_helper::mem {
namespace {

bool ProcessReadable(uintptr_t address, size_t size) {
#if defined(__ANDROID__) || defined(__linux__)
    return ProcMaps::IsReadable(address, size);
#else
    (void)address;
    (void)size;
    return false;
#endif
}

bool ProcessWritable(uintptr_t address, size_t size) {
#if defined(__ANDROID__) || defined(__linux__)
    return ProcMaps::IsWritable(address, size);
#else
    (void)address;
    (void)size;
    return false;
#endif
}

bool ProcessPermissions(uintptr_t address, int &permissions) {
#if defined(__ANDROID__) || defined(__linux__)
    return ProcMaps::GetPermissions(address, permissions);
#else
    (void)address;
    (void)permissions;
    return false;
#endif
}

bool ProcessProtect(uintptr_t address, size_t size, int permissions) {
#if defined(__ANDROID__) || defined(__linux__)
    if (mprotect(reinterpret_cast<void *>(address), size, permissions) != 0) return false;
    ProcMaps::InvalidatePermissionCache();
    return true;
#else
    (void)address;
    (void)size;
    (void)permissions;
    return false;
#endif
}

const MemoryBackend &ProcessBackend() {
    static const MemoryBackend backend{
        .readable = ProcessReadable,
        .writable = ProcessWritable,
        .permissions = ProcessPermissions,
        .protect = ProcessProtect,
    };
    return backend;
}

std::expected<uintptr_t, MemoryError> CheckedEnd(uintptr_t address, size_t size) {
    if (address == 0 || size == 0) return std::unexpected(MemoryError::InvalidRange);
    if (size > std::numeric_limits<uintptr_t>::max() - address) {
        return std::unexpected(MemoryError::Overflow);
    }
    return address + size;
}

} // namespace

RuntimeMemory::RuntimeMemory() : backend_(ProcessBackend()) {}

RuntimeMemory::RuntimeMemory(MemoryBackend backend)
    : backend_(backend.readable && backend.writable && backend.permissions && backend.protect
                   ? backend
                   : ProcessBackend()) {}

RuntimeMemory RuntimeMemory::Process() {
    return RuntimeMemory(ProcessBackend());
}

size_t RuntimeMemory::PageSize() {
#if defined(__ANDROID__) || defined(__linux__)
    const long page_size = sysconf(_SC_PAGESIZE);
    return page_size > 0 ? static_cast<size_t>(page_size) : 4096u;
#else
    return 4096u;
#endif
}

std::expected<void, MemoryError> RuntimeMemory::ReadBytes(
    uintptr_t address, std::span<std::byte> output) const {
    if (output.data() == nullptr) return std::unexpected(MemoryError::InvalidRange);
    const auto end = CheckedEnd(address, output.size());
    if (!end) return std::unexpected(end.error());
    if (!backend_.readable(address, output.size())) {
        return std::unexpected(MemoryError::Unmapped);
    }
    std::memcpy(output.data(), reinterpret_cast<const void *>(address), output.size());
    return {};
}

std::expected<void, MemoryError> RuntimeMemory::WriteBytes(
    uintptr_t address, std::span<const std::byte> input) const {
    if (input.data() == nullptr) return std::unexpected(MemoryError::InvalidRange);
    const auto end = CheckedEnd(address, input.size());
    if (!end) return std::unexpected(end.error());
    if (!backend_.writable(address, input.size())) {
        return std::unexpected(MemoryError::PermissionDenied);
    }
    std::memcpy(reinterpret_cast<void *>(address), input.data(), input.size());
    return {};
}

std::expected<int, MemoryError> RuntimeMemory::GetPermissions(uintptr_t address) const {
    if (address == 0) return std::unexpected(MemoryError::InvalidRange);
    int permissions = 0;
    if (!backend_.permissions(address, permissions)) {
        return std::unexpected(MemoryError::Unmapped);
    }
    return permissions;
}

std::expected<void, MemoryError> RuntimeMemory::Protect(
    uintptr_t address, size_t size, int permissions) const {
    const auto end = CheckedEnd(address, size);
    if (!end) return std::unexpected(end.error());
    if (!backend_.protect(address, size, permissions)) {
        return std::unexpected(MemoryError::ProtectionChangeFailed);
    }
    return {};
}

} // namespace arc_helper::mem
