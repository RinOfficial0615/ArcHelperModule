#include "features/CxaThrowTracer.hpp"

#include <array>
#include <cinttypes>
#include <cstdint>
#include <cstring>

#include <unwind.h>

#include "config/ModuleConfig.h"
#include "manager/HookManager.hpp"
#include "utils/Log.h"
#include "utils/MemoryUtils.hpp"

namespace arc_helper {
namespace {

constexpr std::array<uint8_t, 16> kSigCxaThrow = {
    0x3F, 0x23, 0x03, 0xD5, 0xFD, 0x7B, 0xBC, 0xA9,
    0xF7, 0x0B, 0x00, 0xF9, 0xF6, 0x57, 0x02, 0xA9,
};

uintptr_t g_lib_base = 0;

struct CxaTypeInfoView {
    const void *vtable;
    const char *name;
};

struct ThrowTrace {
    std::array<uintptr_t, 16> frames{};
    size_t size = 0;
};

_Unwind_Reason_Code CollectThrowFrame(_Unwind_Context *context, void *argument) {
    auto &trace = *static_cast<ThrowTrace *>(argument);
    if (trace.size == trace.frames.size()) return _URC_END_OF_STACK;
    const uintptr_t ip = static_cast<uintptr_t>(_Unwind_GetIP(context));
    if (ip) trace.frames[trace.size++] = ip;
    return _URC_NO_REASON;
}

} // namespace

CxaThrowTracer &CxaThrowTracer::Instance() {
    static CxaThrowTracer feature;
    return feature;
}

CxaThrowTracer::CxaThrowTracer() : Feature("CxaThrowTracer") {}

void CxaThrowTracer::Install(const cfg::GameProfile &profile) {
    (void)profile;
    if (installed_) return;

    auto &hook_manager = HookManager::Instance();
    if (!hook_manager.EnsureReady()) {
        ARC_LOGE("__cxa_throw install skipped: %s base not ready", cfg::module::kLibName);
        return;
    }
    g_lib_base = hook_manager.GetLibBase();

    uintptr_t cxa_throw = 0;
    auto registration = hook_manager.RegisterInlineHookSymbol(cxa_throw,
                                                              cfg::module::kLibName,
                                                              "__cxa_throw",
                                                              kSigCxaThrow,
                                                              CxaThrowHook,
                                                              "__cxa_throw");
    if (!registration) {
        ARC_LOGE("__cxa_throw resolution failed");
        return;
    }
    if (!hook_manager.CommitInlineHook(registration)) {
        ARC_LOGE("__cxa_throw hook commit failed");
        return;
    }

    installed_ = true;
    ARC_LOGI("__cxa_throw trace install OK");
}

[[noreturn]] void CxaThrowTracer::CxaThrowHook(void *exception,
                                               const void *type_info,
                                               void (*destructor)(void *)) {
    static thread_local bool tracing = false;
    if (!tracing) {
        tracing = true;
        std::array<char, 128> type_name{};
        std::strcpy(type_name.data(), "<unknown>");
        if (type_info && mem::ProcMaps::IsReadable(reinterpret_cast<uintptr_t>(type_info),
                                                   sizeof(CxaTypeInfoView))) {
            const auto *type = static_cast<const CxaTypeInfoView *>(type_info);
            const char *name = nullptr;
            std::memcpy(&name, &type->name, sizeof(name));
            if (name) {
                size_t length = 0;
                while (length + 1 < type_name.size() &&
                       mem::ProcMaps::IsReadable(reinterpret_cast<uintptr_t>(name + length), 1) &&
                       name[length] != '\0') {
                    type_name[length] = name[length];
                    ++length;
                }
                if (length != 0) type_name[length] = '\0';
            }
        }
        const uintptr_t caller = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
        const uintptr_t caller_offset = caller >= g_lib_base ? caller - g_lib_base : 0;
        ThrowTrace trace;
        _Unwind_Backtrace(CollectThrowFrame, &trace);
        ARC_LOGE("throw type=%s exception=%p caller=%p rel=0x%" PRIxPTR,
                 type_name.data(), exception, reinterpret_cast<void *>(caller), caller_offset);
        for (size_t i = 0; i < trace.size; ++i) {
            const uintptr_t frame = trace.frames[i];
            const uintptr_t offset = frame >= g_lib_base ? frame - g_lib_base : 0;
            ARC_LOGE("throw frame[%zu]=%p rel=0x%" PRIxPTR,
                     i, reinterpret_cast<void *>(frame), offset);
        }
        tracing = false;
    }
    CALL_ORIG(CxaThrowHook, exception, type_info, destructor);
    __builtin_unreachable();
}

} // namespace arc_helper
