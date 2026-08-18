#include "features/SslPinningBypass.hpp"

#include "utils/Log.h"
#include "utils/MemoryUtils.hpp"

#include <array>
#include <cstring>

namespace arc_helper {
namespace {

bool IsCbzRegister20(uint32_t instruction) {
    // Ignore sf (W/X width) but require CBZ rather than CBNZ and Rt == 20.
    return (instruction & 0x7F00001Fu) == 0x34000014u;
}

bool IsBranchWithLink(uint32_t instruction) {
    return (instruction & 0xFC000000u) == 0x94000000u;
}

uint32_t CbzToUnconditionalBranch(uint32_t instruction) {
    uint32_t immediate = (instruction >> 5u) & 0x7FFFFu;
    if ((immediate & 0x40000u) != 0) immediate |= 0x03F80000u;
    return 0x14000000u | (immediate & 0x03FFFFFFu);
}

} // namespace

SslPinningBypass &SslPinningBypass::Instance() {
    static SslPinningBypass instance;
    return instance;
}

SslPinningBypass::SslPinningBypass() : Feature("SslPinningBypass") {}

void SslPinningBypass::Install(const cfg::GameProfile &profile) {
    if (patched_) return;
    if (!enabled_) return;
    auto &hook_manager = HookManager::Instance();
    if (!hook_manager.EnsureReady()) return;
    lib_base_ = hook_manager.GetLibBase();
    if (!lib_base_) return;

    const auto &pins = profile.ssl_pins;
    if (!pins.skip_cbz || !pins.tail_call || !pins.expected_skip_cbz ||
        !pins.expected_tail_call) {
        ARC_LOGE("No patch offsets for this version");
        return;
    }

    if (pins.skip_cbz > UINTPTR_MAX - lib_base_ ||
        pins.tail_call > UINTPTR_MAX - lib_base_) {
        ARC_LOGE("Patch address overflow");
        return;
    }

    const uintptr_t addr_a = lib_base_ + pins.skip_cbz;
    const uintptr_t addr_b = lib_base_ + pins.tail_call;

    if (!mem::ProcMaps::IsReadable(addr_a, 4) || !mem::ProcMaps::IsReadable(addr_b, 4)) {
        ARC_LOGE("Patch addresses not readable");
        return;
    }

    const auto current_a = mem::RuntimeMemory::Process().Read<uint32_t>(addr_a);
    const auto current_b = mem::RuntimeMemory::Process().Read<uint32_t>(addr_b);
    if (!current_a || !current_b) {
        ARC_LOGE("Failed to read patch instructions");
        return;
    }

    if (*current_a != pins.expected_skip_cbz || *current_b != pins.expected_tail_call ||
        !IsCbzRegister20(*current_a) || !IsBranchWithLink(*current_b)) {
        ARC_LOGE("Instruction signature mismatch");
        return;
    }

    const uint32_t patch_a = CbzToUnconditionalBranch(*current_a);
    constexpr uint32_t kPatchB = 0xD503201F;
    std::array<std::byte, sizeof(uint32_t)> expected_a{};
    std::array<std::byte, sizeof(uint32_t)> expected_b{};
    std::array<std::byte, sizeof(uint32_t)> replacement_a{};
    std::array<std::byte, sizeof(uint32_t)> replacement_b{};
    std::memcpy(expected_a.data(), &*current_a, expected_a.size());
    std::memcpy(expected_b.data(), &*current_b, expected_b.size());
    std::memcpy(replacement_a.data(), &patch_a, replacement_a.size());
    std::memcpy(replacement_b.data(), &kPatchB, replacement_b.size());
    const std::array<mem::PatchDescriptor, 2> patches = {{
        {addr_a, std::span<const std::byte>(expected_a), std::span<const std::byte>(replacement_a)},
        {addr_b, std::span<const std::byte>(expected_b), std::span<const std::byte>(replacement_b)},
    }};
    if (!patch_transaction_.Apply(patches)) {
        if (patch_transaction_.IsDegraded() && !patch_transaction_.Rollback()) {
            ARC_LOGE("Degraded patch rollback failed");
        }
        ARC_LOGE("Patch transaction failed");
        return;
    }

    patched_ = true;
    ARC_LOGI("Patched @ %p / %p",
             reinterpret_cast<void *>(addr_a),
             reinterpret_cast<void *>(addr_b));
}

} // namespace arc_helper
