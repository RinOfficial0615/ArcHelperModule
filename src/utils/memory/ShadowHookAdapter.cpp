#include "utils/memory/ShadowHookAdapter.hpp"

#include <atomic>
#include <dlfcn.h>
#include <string>
#include <string_view>

#include "shadowhook.h"
#include "utils/Log.h"

#if defined(__ANDROID__)
extern "C" void *__real_dlopen(const char *filename, int flags);
#endif

namespace arc_helper::mem {
namespace {

std::atomic<const char *> g_shadowhook_nothing_path{nullptr};

std::string HelperPath() {
    Dl_info info{};
    if (!dladdr(reinterpret_cast<void *>(&HelperPath), &info) || !info.dli_fname) {
        return {};
    }
    const std::string module_path(info.dli_fname);
    const size_t slash = module_path.find_last_of('/');
    return slash == std::string::npos
               ? std::string("libshadowhook_nothing.so")
               : module_path.substr(0, slash + 1) + "libshadowhook_nothing.so";
}

bool EnsureShadowHook() {
    // shadowhook_init() is idempotent and internally serialized. Cache only a
    // successful initialization so a transient early failure can be retried.
    static std::atomic_bool initialized{false};
    if (initialized.load(std::memory_order_acquire)) return true;

    // ShadowHook must load the helper after it hooks call_constructors.
    // Loading it here would make the later dlopen return an already-loaded
    // soinfo and skip the constructor event used by the layout scanner.
    static const std::string helper_path = HelperPath();
    g_shadowhook_nothing_path.store(helper_path.empty() ? nullptr : helper_path.c_str(),
                                    std::memory_order_release);

    const int error = shadowhook_init(SHADOWHOOK_MODE_UNIQUE, false);
    if (error != SHADOWHOOK_ERRNO_OK) {
        ARC_LOGE("ShadowHook init failed: %d (%s)", error, shadowhook_to_errmsg(error));
        return false;
    }

    initialized.store(true, std::memory_order_release);
    return true;
}

} // namespace

#if defined(__ANDROID__)
extern "C" void *__wrap_dlopen(const char *filename, int flags) {
    constexpr std::string_view kHelperName = "libshadowhook_nothing.so";
    if (filename && std::string_view(filename) == kHelperName) {
        if (const char *path = g_shadowhook_nothing_path.load(std::memory_order_acquire)) {
            return __real_dlopen(path, flags);
        }
    }
    return __real_dlopen(filename, flags);
}
#endif

bool ShadowHookAdapter::Install(uintptr_t target,
                                void *hook_fn,
                                void **original_fn_out,
                                void **stub_out) {
    if (!target || !hook_fn || !original_fn_out || !stub_out || !EnsureShadowHook()) {
        return false;
    }

    *original_fn_out = nullptr;
    *stub_out = shadowhook_hook_func_addr(reinterpret_cast<void *>(target),
                                          hook_fn,
                                          original_fn_out);
    if (!*stub_out || !*original_fn_out) {
        const int error = shadowhook_get_errno();
        ARC_LOGE("ShadowHook hook failed at %p: %d (%s)",
                 reinterpret_cast<void *>(target),
                 error,
                 shadowhook_to_errmsg(error));
        *original_fn_out = nullptr;
        *stub_out = nullptr;
        return false;
    }
    return true;
}

bool ShadowHookAdapter::Uninstall(void *stub) {
    if (!stub) return false;
    const int error = shadowhook_unhook(stub);
    if (error != SHADOWHOOK_ERRNO_OK) {
        ARC_LOGE("ShadowHook unhook failed: %d (%s)", error, shadowhook_to_errmsg(error));
        return false;
    }
    return true;
}

} // namespace arc_helper::mem
