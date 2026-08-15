#include <jni.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>

#include "wrapper/WrapperCommon.hpp"

namespace {

// One-time JNI-side init gate for this .so instance.
std::atomic<bool> g_jni_wrapper_inited = false;
std::atomic<bool> g_jni_retry_started = false;

// ReLinker loads this library before the game's version callback.  Generate
// the user-editable runtime config at load time so a fresh install always has
// a readable, beautified baseline even when offset resolution is still
// pending.
__attribute__((constructor)) void GenerateDefaultRuntimeConfigAtLoad() {
    arc_helper::RuntimeConfig::Instance().EnsureLoaded();
}

std::string PrepareRuntimeRoot(JavaVM *vm) {
    if (!vm) return {};
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK || !env) {
        return {};
    }

    jclass activity_thread = env->FindClass("android/app/ActivityThread");
    if (!activity_thread) return {};
    jmethodID current_application = env->GetStaticMethodID(
        activity_thread, "currentApplication", "()Landroid/app/Application;");
    if (!current_application) {
        env->DeleteLocalRef(activity_thread);
        return {};
    }
    jobject application = env->CallStaticObjectMethod(activity_thread, current_application);
    if (env->ExceptionCheck() || !application) {
        env->ExceptionClear();
        env->DeleteLocalRef(activity_thread);
        return {};
    }

    jclass context = env->GetObjectClass(application);
    jmethodID get_external_files_dir = env->GetMethodID(
        context, "getExternalFilesDir", "(Ljava/lang/String;)Ljava/io/File;");
    jobject external_files_dir = get_external_files_dir
        ? env->CallObjectMethod(application, get_external_files_dir, nullptr)
        : nullptr;
    if (env->ExceptionCheck() || !external_files_dir) {
        env->ExceptionClear();
        env->DeleteLocalRef(context);
        env->DeleteLocalRef(application);
        env->DeleteLocalRef(activity_thread);
        return {};
    }

    jclass file_class = env->FindClass("java/io/File");
    jmethodID get_absolute_path = file_class
        ? env->GetMethodID(file_class, "getAbsolutePath", "()Ljava/lang/String;")
        : nullptr;
    jstring external_path = get_absolute_path
        ? static_cast<jstring>(env->CallObjectMethod(external_files_dir, get_absolute_path))
        : nullptr;
    if (env->ExceptionCheck() || !external_path) {
        env->ExceptionClear();
        if (file_class) env->DeleteLocalRef(file_class);
        env->DeleteLocalRef(external_files_dir);
        env->DeleteLocalRef(context);
        env->DeleteLocalRef(application);
        env->DeleteLocalRef(activity_thread);
        return {};
    }

    const char *path_utf8 = env->GetStringUTFChars(external_path, nullptr);
    const std::string root = path_utf8 ? std::string(path_utf8) + "/ArcHelper" : std::string{};
    if (path_utf8) env->ReleaseStringUTFChars(external_path, path_utf8);

    if (file_class) {
        jmethodID file_ctor = env->GetMethodID(file_class, "<init>", "(Ljava/lang/String;)V");
        jmethodID mkdirs = env->GetMethodID(file_class, "mkdirs", "()Z");
        if (file_ctor && mkdirs && !root.empty()) {
            jstring root_string = env->NewStringUTF(root.c_str());
            jobject root_file = env->NewObject(file_class, file_ctor, root_string);
            if (root_file) env->CallBooleanMethod(root_file, mkdirs);
            if (env->ExceptionCheck()) env->ExceptionClear();
            if (root_file) env->DeleteLocalRef(root_file);
            env->DeleteLocalRef(root_string);
        }
        env->DeleteLocalRef(file_class);
    }
    env->DeleteLocalRef(external_path);
    env->DeleteLocalRef(external_files_dir);
    env->DeleteLocalRef(context);
    env->DeleteLocalRef(application);
    env->DeleteLocalRef(activity_thread);
    return root;
}

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
    (void)reserved;

    const std::string runtime_root = PrepareRuntimeRoot(vm);
    if (!runtime_root.empty()) {
        arc_helper::RuntimeConfig::Instance().SetRootDir(runtime_root);
    }
    arc_helper::RuntimeConfig::Instance().EnsureLoaded();

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
