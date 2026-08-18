#pragma once

#include <atomic>
#include <cstdint>
#include <string_view>

namespace arc_helper {

// Small cross-hook state machine for the custom-chart gameplay window.
// Asset hooks publish events, while consumers only read an atomic snapshot.
class CustomChartGameplaySession final {
public:
    struct Snapshot {
        bool active = false;
        uint64_t generation = 0;
    };

    static CustomChartGameplaySession &Instance();

    // Starts a session. OpenHook only publishes paths that
    // CustomChartAssetIndex::IsCustomChartPath already accepted.
    void OnCustomChartMapped(std::string_view logical_path);

    // A read of any asset path ending in "base.jpg" closes the session.
    void OnAssetRead(std::string_view logical_path);

    Snapshot Read() const noexcept;
    bool IsActive() const noexcept { return Read().active; }

#ifdef ARC_HELPER_HOST_TEST
    void ResetForTesting() noexcept { state_.store(0, std::memory_order_release); }
#endif

private:
    static constexpr uint64_t kActiveMask = uint64_t{1} << 63;
    static constexpr uint64_t kGenerationMask = ~kActiveMask;

    CustomChartGameplaySession() = default;

    static bool IsBaseAsset(std::string_view logical_path) noexcept;

    std::atomic_uint64_t state_{0};
};

} // namespace arc_helper
