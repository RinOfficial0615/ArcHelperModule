#pragma once

#include <atomic>
#include <cstdint>

namespace arc_helper {

enum class CustomSessionState : uint8_t {
    Official = 0,
    CustomSelected,
    CustomPlaying,
    CustomResult,
};

class CustomSession {
public:
    static CustomSession &Instance();

    // Lifecycle entry points. They are called by the selected-song/load and
    // scene-transition hooks, never by official asset probes.
    void OnSongSelected(const char *song_id);
    void OnLoadFailure(const char *reason);
    void OnResultEntered();
    void OnResultExited();
    void OnSelectionScreenEntered();
    void MarkPlaying();
    void MarkResult();
    void Clear(const char *reason);

    bool IsActive() const { return state_.load(std::memory_order_acquire) != CustomSessionState::Official; }
    CustomSessionState State() const { return state_.load(std::memory_order_acquire); }

private:
    CustomSession() = default;
    std::atomic<CustomSessionState> state_{CustomSessionState::Official};
};

} // namespace arc_helper
