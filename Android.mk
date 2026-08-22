LOCAL_PATH := $(call my-dir)

# Vendored hooking engines (shadowhook, lsplt). Third-party code is exempt
# from the repo-wide warnings-as-failures policy (AGENTS.md).
include $(CLEAR_VARS)
LOCAL_MODULE := arc_helper_ext
LOCAL_SRC_FILES := \
    third_party/shadowhook/shadowhook/src/main/cpp/arch/arm64/sh_a64.c \
    third_party/shadowhook/shadowhook/src/main/cpp/arch/arm64/sh_glue.S \
    third_party/shadowhook/shadowhook/src/main/cpp/arch/arm64/sh_inst.c \
    third_party/shadowhook/shadowhook/src/main/cpp/common/bytesig.c \
    third_party/shadowhook/shadowhook/src/main/cpp/common/sh_errno.c \
    third_party/shadowhook/shadowhook/src/main/cpp/common/sh_log.c \
    third_party/shadowhook/shadowhook/src/main/cpp/common/sh_ref.c \
    third_party/shadowhook/shadowhook/src/main/cpp/common/sh_trampo.c \
    third_party/shadowhook/shadowhook/src/main/cpp/common/sh_util.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_elf.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_enter.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_hub.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_island.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_linker.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_recorder.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_safe.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_switch.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_task.c \
    third_party/shadowhook/shadowhook/src/main/cpp/sh_xdl.c \
    third_party/shadowhook/shadowhook/src/main/cpp/shadowhook.c \
    third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl/xdl.c \
    third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl/xdl_iterate.c \
    third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl/xdl_linker.c \
    third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl/xdl_lzma.c \
    third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl/xdl_util.c \
    build/generated/lsplt/lsplt/src/main/jni/lsplt.cc \
    build/generated/lsplt/lsplt/src/main/jni/elf_util.cc
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/include \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/arch/arm64 \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/common \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/third_party/bsd \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/third_party/lss \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl \
    $(LOCAL_PATH)/build/generated/lsplt/lsplt/src/main/jni \
    $(LOCAL_PATH)/build/generated/lsplt/lsplt/src/main/jni/include
LOCAL_CONLYFLAGS += -std=c11 -fvisibility=hidden
LOCAL_CPPFLAGS += -DC4_NO_DEBUG_BREAK -DC4_USE_ASSERT=0 -D_LIBCPP_ABI_NAMESPACE=_LIBCPP_ABI_NAMESPACE
LOCAL_STATIC_LIBRARIES := libcxx
include $(BUILD_STATIC_LIBRARY)

# First-party module code. AGENTS.md: treat warnings as failures.
include $(CLEAR_VARS)
LOCAL_MODULE := arc_helper
LOCAL_SRC_FILES := \
    src/wrapper/ZygiskEntryWrapper.cpp \
    src/wrapper/JniEntryWrapper.cpp \
    src/features/Autoplay.cpp \
    src/features/CxaThrowTracer.cpp \
    src/features/Logging.cpp \
    src/features/NetworkLogger.cpp \
    src/features/NetworkBlock.cpp \
    src/features/SslPinningBypass.cpp \
    src/features/CustomCharts.cpp \
    src/features/AssetVirtualizer.cpp \
    src/manager/ConfigManager.cpp \
    src/manager/FeatureManager.cpp \
    src/manager/GameManager.cpp \
    src/manager/CustomChartManager.cpp \
    src/manager/custom_chart/CustomChartGameplaySession.cpp \
    src/manager/custom_chart/CustomChartImporter.cpp \
    src/manager/custom_chart/ArcPackageFormat.cpp \
    src/manager/custom_chart/AffNormalizer.cpp \
    src/manager/custom_chart/CustomChartAssetIndex.cpp \
    src/manager/custom_chart/CustomChartSnapshot.cpp \
    src/manager/custom_chart/CustomChartReportWriter.cpp \
    src/manager/GameVersionManager.cpp \
    src/manager/NetworkManager.cpp \
    src/manager/network/NetworkHandlerSnapshot.cpp \
    src/manager/HookManager.cpp \
    src/utils/memory/ProcMaps.cpp \
    src/utils/memory/AddressResolver.cpp \
    src/utils/memory/InlineHook.cpp \
    src/utils/memory/RuntimeMemory.cpp \
    src/utils/memory/PatchTransaction.cpp \
    src/utils/memory/ShadowHookAdapter.cpp \
    src/utils/memory/ExecUtils.cpp \
    src/utils/Sha256.cpp \
    src/utils/Log.cpp \
    src/utils/ZipArchive.cpp \
    src/utils/ImageRaster.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/third_party/json/include \
    $(LOCAL_PATH)/third_party/magic_enum/include \
    $(LOCAL_PATH)/third_party/rapidyaml/src \
    $(LOCAL_PATH)/third_party/rapidyaml/ext/c4core.src \
    $(LOCAL_PATH)/third_party \
    $(LOCAL_PATH)/build/generated/lsplt/lsplt/src/main/jni \
    $(LOCAL_PATH)/build/generated/lsplt/lsplt/src/main/jni/include \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/include
# Vendored rapidyaml/c4core headers carry "-Wnrvo" pragmas that newer clang
# rejects under -Werror; third-party code stays exempt from warnings policy.
LOCAL_CPPFLAGS += -Wall -Wextra -Werror -Wno-unknown-warning-option \
    -DC4_NO_DEBUG_BREAK -DC4_USE_ASSERT=0 -D_LIBCPP_ABI_NAMESPACE=_LIBCPP_ABI_NAMESPACE
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384 -Wl,--wrap=dlopen
LOCAL_LDLIBS += -llog -landroid -ldl -lz
LOCAL_STATIC_LIBRARIES := arc_helper_ext rapidyaml libcxx
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := rapidyaml
LOCAL_SRC_FILES := \
    third_party/rapidyaml/ext/c4core.src/c4/base64.cpp \
    third_party/rapidyaml/ext/c4core.src/c4/error.cpp \
    third_party/rapidyaml/ext/c4core.src/c4/format.cpp \
    third_party/rapidyaml/ext/c4core.src/c4/language.cpp \
    third_party/rapidyaml/ext/c4core.src/c4/memory_util.cpp \
    third_party/rapidyaml/ext/c4core.src/c4/utf.cpp \
    third_party/rapidyaml/ext/c4core.src/c4/version.cpp \
    third_party/rapidyaml/src/c4/yml/common.cpp \
    third_party/rapidyaml/src/c4/yml/node_type.cpp \
    third_party/rapidyaml/src/c4/yml/parse.cpp \
    third_party/rapidyaml/src/c4/yml/reference_resolver.cpp \
    third_party/rapidyaml/src/c4/yml/scalar_style.cpp \
    third_party/rapidyaml/src/c4/yml/tag.cpp \
    third_party/rapidyaml/src/c4/yml/tree.cpp \
    third_party/rapidyaml/src/c4/yml/version.cpp
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/third_party/rapidyaml/src \
    $(LOCAL_PATH)/third_party/rapidyaml/ext/c4core.src
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)
# Vendored headers use "-Wnrvo" pragmas that newer clang warns about.
LOCAL_CPPFLAGS += -Wno-unknown-warning-option \
    -DC4_NO_DEBUG_BREAK -DC4_USE_ASSERT=0 -D_LIBCPP_ABI_NAMESPACE=_LIBCPP_ABI_NAMESPACE -include vector
LOCAL_STATIC_LIBRARIES := libcxx
include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := shadowhook_nothing
LOCAL_SRC_FILES := module/shadowhook_nothing.c
LOCAL_CFLAGS += -Oz -fno-ident -fno-unwind-tables -fno-asynchronous-unwind-tables
LOCAL_LDFLAGS += \
    -Wl,--strip-all \
    -Wl,--as-needed \
    -Wl,--hash-style=sysv \
    -Wl,--build-id=none \
    -Wl,-z,max-page-size=16384 \
    -Wl,-nostdlib -nostartfiles -nodefaultlibs
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/third_party/libcxx/Android.mk
