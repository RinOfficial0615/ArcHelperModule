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

    void Activate(const char *song_id);
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
