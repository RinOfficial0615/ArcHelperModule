#pragma once

#include <cstdint>

namespace arc_helper::mem {

class InlineHook {
public:
    static bool InstallA64(uintptr_t target,
                           void *hook_fn,
                           void **orig_fn_out,
                           void **stub_out);
    static bool RestoreA64(void *stub);
};

} // namespace arc_helper::mem
