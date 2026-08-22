#include "utils/memory/InlineHook.hpp"

#if defined(__ANDROID__)

#include "utils/memory/ShadowHookAdapter.hpp"

namespace arc_helper::mem {

bool InlineHook::InstallA64(uintptr_t target,
                            void *hook_fn,
                            void **orig_fn_out,
                            void **stub_out) {
    return ShadowHookAdapter::Install(target, hook_fn, orig_fn_out, stub_out);
}

bool InlineHook::RestoreA64(void *stub) {
    // ShadowHook owns the trampoline lifetime; the opaque stub is the only
    // valid handle for unhooking. Callers keep the original pointer solely
    // for invoking the trampoline.
    return ShadowHookAdapter::Uninstall(stub);
}

} // namespace arc_helper::mem

#else

// Host builds do not execute AArch64 hooks. Keeping a fail-closed backend
// makes accidental host installation attempts explicit and linkable.
namespace arc_helper::mem {

bool InlineHook::InstallA64(uintptr_t, void *, void **orig_fn_out, void **stub_out) {
    if (orig_fn_out) *orig_fn_out = nullptr;
    if (stub_out) *stub_out = nullptr;
    return false;
}

bool InlineHook::RestoreA64(void *) {
    return false;
}

} // namespace arc_helper::mem

#endif
