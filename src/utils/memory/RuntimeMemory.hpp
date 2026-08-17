#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <type_traits>

#include "utils/memory/MemoryError.hpp"

namespace arc_helper::mem {

struct MemoryBackend {
    using RangeCheck = bool (*)(uintptr_t address, size_t size);
    using Permissions = bool (*)(uintptr_t address, int &permissions);
    using Protect = bool (*)(uintptr_t address, size_t size, int permissions);

    RangeCheck readable = nullptr;
    RangeCheck writable = nullptr;
    Permissions permissions = nullptr;
    Protect protect = nullptr;
};

class RuntimeMemory {
public:
    RuntimeMemory();
    explicit RuntimeMemory(MemoryBackend backend);

    static RuntimeMemory Process();
    static size_t PageSize();

    std::expected<void, MemoryError> ReadBytes(uintptr_t address,
                                               std::span<std::byte> output) const;
    std::expected<void, MemoryError> WriteBytes(uintptr_t address,
                                                std::span<const std::byte> input) const;
    std::expected<int, MemoryError> GetPermissions(uintptr_t address) const;
    std::expected<void, MemoryError> Protect(uintptr_t address,
                                             size_t size,
                                             int permissions) const;

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::expected<T, MemoryError> Read(uintptr_t address) const {
        if (address == 0 || address % alignof(T) != 0) {
            return std::unexpected(address == 0 ? MemoryError::InvalidRange
                                                : MemoryError::Misaligned);
        }
        T value{};
        auto bytes = std::as_writable_bytes(std::span<T>(&value, 1));
        if (auto result = ReadBytes(address, bytes); !result) return std::unexpected(result.error());
        return value;
    }

    template <typename T>
        requires std::is_trivially_copyable_v<T>
    std::expected<void, MemoryError> Write(uintptr_t address, const T &value) const {
        if (address == 0 || address % alignof(T) != 0) {
            return std::unexpected(address == 0 ? MemoryError::InvalidRange
                                                : MemoryError::Misaligned);
        }
        return WriteBytes(address, std::as_bytes(std::span<const T>(&value, 1)));
    }

private:
    MemoryBackend backend_{};
};

} // namespace arc_helper::mem
