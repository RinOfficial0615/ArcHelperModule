#pragma once

#include "features/Feature.hpp"
#include "game/GameProfile.hpp"

namespace arc_helper {

// Always-on diagnostic that logs every C++ exception thrown through the game
// library with its type name, caller, and a short unwind trace. It has no
// configuration and cannot be disabled.
class CxaThrowTracer final : public Feature {
public:
    static CxaThrowTracer &Instance();
    void Install(const cfg::GameProfile &profile);

private:
    CxaThrowTracer();

    [[noreturn]] static void CxaThrowHook(void *exception,
                                          const void *type_info,
                                          void (*destructor)(void *));

    bool installed_ = false;
};

} // namespace arc_helper
