#pragma once

#include <cstdint>

#include "features/Feature.hpp"
#include "game/GameProfile.hpp"
#include "manager/HookManager.hpp"
#include "utils/memory/PatchTransaction.hpp"

namespace arc_helper {

class SslPinningBypass : public Feature {
public:
    static SslPinningBypass &Instance();
    void Install(const cfg::GameProfile &profile);

private:
    SslPinningBypass();

    bool patched_ = false;
    mem::PatchTransaction patch_transaction_;
    uintptr_t lib_base_ = 0;
    AH_CFG(enabled, false);
};

} // namespace arc_helper
