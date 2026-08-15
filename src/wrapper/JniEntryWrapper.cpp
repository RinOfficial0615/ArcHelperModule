#include <jni.h>

#include <atomic>
#include <cstdint>

#include "wrapper/WrapperCommon.hpp"

namespace {

// One-time JNI-side init gate for this .so instance.
std::atomic<bool> g_jni_wrapper_inited = false;

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)vm;
    (void)reserved;

    if (g_jni_wrapper_inited.load(std::memory_order_relaxed)) {
        return JNI_VERSION_1_6;
    }

    const uintptr_t lib_base = arc_helper::wrapper::FindGameLibraryBase();
    if (!lib_base) {
        ARC_LOGE("JNI wrapper: %s not found; skip init", arc_helper::cfg::module::kLibName);
        return JNI_VERSION_1_6;
    }

    ARC_LOGI("JNI wrapper: found %s base @ %p", arc_helper::cfg::module::kLibName, reinterpret_cast<void *>(lib_base));
    arc_helper::wrapper::InitFeatures();
    g_jni_wrapper_inited.store(true, std::memory_order_relaxed);
    return JNI_VERSION_1_6;
}
