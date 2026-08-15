#include "features/CustomSession.hpp"

#include "utils/Log.h"

namespace arc_helper {

CustomSession &CustomSession::Instance() {
    static CustomSession session;
    return session;
}

void CustomSession::Activate(const char *song_id) {
    state_.store(CustomSessionState::CustomSelected, std::memory_order_release);
    ARC_LOGI("CustomSession: active song=%s", song_id ? song_id : "(unknown)");
}

void CustomSession::MarkPlaying() {
    if (IsActive()) state_.store(CustomSessionState::CustomPlaying, std::memory_order_release);
}

void CustomSession::MarkResult() {
    if (IsActive()) state_.store(CustomSessionState::CustomResult, std::memory_order_release);
}

void CustomSession::Clear(const char *reason) {
    const auto previous = state_.exchange(CustomSessionState::Official, std::memory_order_acq_rel);
    if (previous != CustomSessionState::Official) {
        ARC_LOGI("CustomSession: cleared reason=%s", reason ? reason : "unknown");
    }
}

} // namespace arc_helper
