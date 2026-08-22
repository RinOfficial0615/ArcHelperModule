#pragma once

#include <jni.h>

#include "utils/Log.h"

namespace arc_helper {

// Logs and clears any pending JNI exception so it cannot leak into later
// calls. `where` names the JNI site that noticed the pending exception.
inline void ClearPendingJniException(JNIEnv *env, const char *where) {
    if (!env || !env->ExceptionCheck()) return;
    ARC_LOGE("JNI exception at %s", where ? where : "unknown");
    env->ExceptionClear();
}

} // namespace arc_helper
