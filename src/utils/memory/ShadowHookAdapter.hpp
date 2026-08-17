#pragma once

#include <cstdint>

namespace arc_helper::mem {

// Small adapter around ByteDance ShadowHook. The returned stub is required by
// Uninstall; the original pointer is the callable trampoline.
class ShadowHookAdapter {
public:
    static bool Install(uintptr_t target,
                        void *hook_fn,
                        void **original_fn_out,
                        void **stub_out);
    static bool Uninstall(void *stub);
};

} // namespace arc_helper::mem
