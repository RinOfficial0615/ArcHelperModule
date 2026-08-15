#include <jni.h>

#include <atomic>
#include <cstdint>
#include <thread>
#include <chrono>
#include <unistd.h>

#include "wrapper/WrapperCommon.hpp"

namespace {

// One-time JNI-side init gate for this .so instance.
std::atomic<bool> g_jni_wrapper_inited = false;
std::atomic<bool> g_jni_retry_started = false;

void RetryInitAfterGameLibraryLoad() {
    if (g_jni_retry_started.exchange(true, std::memory_order_acq_rel)) return;
    std::thread([] {
        for (int attempt = 0; attempt < 120; ++attempt) {
            const uintptr_t base = arc_helper::wrapper::FindGameLibraryBase();
            if (base) {
                arc_helper::wrapper::InitFeatures();
                if (arc_helper::GameVersionManager::Instance().IsResolved()) {
                    ARC_LOGI("JNI wrapper: delayed init resolved %s @ %p",
                             arc_helper::cfg::module::kLibName, reinterpret_cast<void *>(base));
                    g_jni_wrapper_inited.store(true, std::memory_order_release);
                    return;
                }
            }
            usleep(250000);
        }
        ARC_LOGE("JNI wrapper: timed out waiting for %s", arc_helper::cfg::module::kLibName);
    }).detach();
}

} // namespace

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)vm;
    (void)reserved;

    if (g_jni_wrapper_inited.load(std::memory_order_relaxed)) {
        return JNI_VERSION_1_6;
    }

    const uintptr_t lib_base = arc_helper::wrapper::FindGameLibraryBase();
    if (!lib_base) {
        ARC_LOGI("JNI wrapper: %s not loaded yet; delayed init armed", arc_helper::cfg::module::kLibName);
        RetryInitAfterGameLibraryLoad();
        return JNI_VERSION_1_6;
    }

    ARC_LOGI("JNI wrapper: found %s base @ %p", arc_helper::cfg::module::kLibName, reinterpret_cast<void *>(lib_base));
    arc_helper::wrapper::InitFeatures();
    if (arc_helper::GameVersionManager::Instance().IsResolved()) {
        g_jni_wrapper_inited.store(true, std::memory_order_release);
    } else {
        RetryInitAfterGameLibraryLoad();
    }
    return JNI_VERSION_1_6;
}
