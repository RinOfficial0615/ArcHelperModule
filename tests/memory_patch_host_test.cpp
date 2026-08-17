#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

#include "utils/memory/PatchTransaction.hpp"

namespace {

constexpr size_t kPageSize = 4096;
alignas(kPageSize) std::array<std::byte, kPageSize * 2> g_storage{};
int g_permissions[2] = {0x5, 0x1};
int g_protect_calls = 0;
int g_fail_protect_call = 0;
int g_writable_checks = 0;
int g_fail_writable_check = 0;

bool InRange(uintptr_t address, size_t size) {
    const uintptr_t begin = reinterpret_cast<uintptr_t>(g_storage.data());
    return address >= begin && size <= (begin + g_storage.size()) - address;
}

bool Readable(uintptr_t address, size_t size) {
    return InRange(address, size);
}

bool Writable(uintptr_t address, size_t size) {
    ++g_writable_checks;
    if (g_fail_writable_check != 0 && g_writable_checks == g_fail_writable_check) return false;
    if (!InRange(address, size)) return false;
    const uintptr_t begin = reinterpret_cast<uintptr_t>(g_storage.data());
    const size_t page = static_cast<size_t>((address - begin) / kPageSize);
    return page < 2 && (g_permissions[page] & 0x2) != 0;
}

bool Permissions(uintptr_t address, int &permissions) {
    const uintptr_t begin = reinterpret_cast<uintptr_t>(g_storage.data());
    if (address < begin || address >= begin + g_storage.size()) return false;
    const size_t page = static_cast<size_t>((address - begin) / kPageSize);
    if (page >= 2) return false;
    permissions = g_permissions[page];
    return true;
}

bool Protect(uintptr_t address, size_t size, int permissions) {
    ++g_protect_calls;
    if (g_fail_protect_call != 0 && g_protect_calls == g_fail_protect_call) return false;
    if (size != kPageSize) return false;
    const uintptr_t begin = reinterpret_cast<uintptr_t>(g_storage.data());
    if (address < begin || address >= begin + g_storage.size() ||
        (address - begin) % kPageSize != 0) {
        return false;
    }
    g_permissions[(address - begin) / kPageSize] = permissions;
    return true;
}

arc_helper::mem::RuntimeMemory MakeMemory() {
    return arc_helper::mem::RuntimeMemory({
        .readable = Readable,
        .writable = Writable,
        .permissions = Permissions,
        .protect = Protect,
    });
}

std::span<const std::byte> Bytes(const std::array<std::byte, 4> &value) {
    return std::span<const std::byte>(value.data(), value.size());
}

} // namespace

int main() {
    using arc_helper::mem::PatchDescriptor;
    using arc_helper::mem::PatchTransaction;

    const uintptr_t begin = reinterpret_cast<uintptr_t>(g_storage.data());
    for (size_t i = 0; i < g_storage.size(); ++i) {
        g_storage[i] = static_cast<std::byte>(i & 0xffu);
    }

    const std::array<std::byte, 4> first_expected = {
        g_storage[kPageSize - 2], g_storage[kPageSize - 1], g_storage[kPageSize], g_storage[kPageSize + 1]};
    const std::array<std::byte, 4> first_replacement = {
        std::byte{0xa1}, std::byte{0xa2}, std::byte{0xa3}, std::byte{0xa4}};
    const PatchDescriptor first{
        begin + kPageSize - 2, Bytes(first_expected), Bytes(first_replacement)};

    PatchTransaction transaction(MakeMemory());
    assert(transaction.Apply(std::span<const PatchDescriptor>(&first, 1)));
    assert(transaction.IsApplied());
    assert(std::memcmp(g_storage.data() + kPageSize - 2,
                       first_replacement.data(),
                       first_replacement.size()) == 0);
    assert(g_permissions[0] == 0x5 && g_permissions[1] == 0x1);
    assert(transaction.Rollback());
    assert(transaction.State() == arc_helper::mem::PatchState::RolledBack);
    assert(std::memcmp(g_storage.data() + kPageSize - 2,
                       first_expected.data(),
                       first_expected.size()) == 0);

    const std::array<std::byte, 4> second_expected = {
        g_storage[32], g_storage[33], g_storage[34], g_storage[35]};
    const std::array<std::byte, 4> second_replacement = {
        std::byte{0xb1}, std::byte{0xb2}, std::byte{0xb3}, std::byte{0xb4}};
    const std::array<std::byte, 4> third_expected = {
        g_storage[64], g_storage[65], g_storage[66], g_storage[67]};
    const std::array<std::byte, 4> third_replacement = {
        std::byte{0xc1}, std::byte{0xc2}, std::byte{0xc3}, std::byte{0xc4}};
    const std::array<PatchDescriptor, 2> failing_patches = {{
        {begin + 32, Bytes(second_expected), Bytes(second_replacement)},
        {begin + 64, Bytes(third_expected), Bytes(third_replacement)},
    }};
    g_writable_checks = 0;
    g_fail_writable_check = 2;
    PatchTransaction failing(MakeMemory());
    assert(!failing.Apply(failing_patches));
    assert(failing.State() == arc_helper::mem::PatchState::Ready);
    assert(std::memcmp(g_storage.data() + 32, second_expected.data(), 4) == 0);
    g_fail_writable_check = 0;

    const auto mismatch = PatchDescriptor{begin + 32, Bytes(first_expected), Bytes(first_replacement)};
    assert(!failing.Apply(std::span<const PatchDescriptor>(&mismatch, 1)));
    assert(failing.State() == arc_helper::mem::PatchState::Ready);

    PatchTransaction degraded(MakeMemory());
    assert(degraded.Apply(std::span<const PatchDescriptor>(&first, 1)));
    g_protect_calls = 0;
    g_fail_protect_call = 2;
    assert(!degraded.Rollback());
    assert(degraded.IsDegraded());
    g_fail_protect_call = 0;

    const PatchDescriptor overflow{
        std::numeric_limits<uintptr_t>::max() - 1, Bytes(first_expected), Bytes(first_replacement)};
    assert(!failing.Apply(std::span<const PatchDescriptor>(&overflow, 1)));
    assert(failing.State() == arc_helper::mem::PatchState::Ready);
    assert(failing.Apply(std::span<const PatchDescriptor>(&first, 1)));
    return 0;
}
