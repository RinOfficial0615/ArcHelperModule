#include "manager/custom_chart/CustomChartGameplaySession.hpp"

#include "config/CustomChartConfig.h"
#include "utils/Log.h"

namespace arc_helper {

CustomChartGameplaySession &CustomChartGameplaySession::Instance() {
    static CustomChartGameplaySession session;
    return session;
}

void CustomChartGameplaySession::OnCustomChartMapped(std::string_view logical_path) {
    uint64_t current = state_.load(std::memory_order_acquire);
    for (;;) {
        if ((current & kActiveMask) != 0) return;
        const uint64_t previous_generation = current & kGenerationMask;
        // Keep zero reserved for the never-started state even after a very
        // large number of chart switches.
        const uint64_t generation = previous_generation == kGenerationMask
                                        ? 1
                                        : previous_generation + 1;
        const uint64_t next = (generation & kGenerationMask) | kActiveMask;
        if (state_.compare_exchange_weak(current, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire)) {
            ARC_LOGI("Custom chart gameplay started (%.*s)",
                     static_cast<int>(logical_path.size()), logical_path.data());
            return;
        }
    }
}

void CustomChartGameplaySession::OnAssetRead(std::string_view logical_path) {
    if (!IsBaseAsset(logical_path)) return;

    uint64_t current = state_.load(std::memory_order_acquire);
    for (;;) {
        if ((current & kActiveMask) == 0) return;
        const uint64_t next = current & kGenerationMask;
        if (state_.compare_exchange_weak(current, next,
                                          std::memory_order_acq_rel,
                                          std::memory_order_acquire)) {
            ARC_LOGI("Custom chart gameplay ended (%.*s)",
                     static_cast<int>(logical_path.size()), logical_path.data());
            return;
        }
    }
}

bool CustomChartGameplaySession::IsBaseAsset(std::string_view logical_path) noexcept {
    return logical_path.ends_with(cfg::custom_charts::kJacketAssetName);
}

CustomChartGameplaySession::Snapshot CustomChartGameplaySession::Read() const noexcept {
    const uint64_t state = state_.load(std::memory_order_acquire);
    return {
        .active = (state & kActiveMask) != 0,
        .generation = state & kGenerationMask,
    };
}

} // namespace arc_helper
