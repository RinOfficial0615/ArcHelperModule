LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := arc_helper
LOCAL_SRC_FILES := \
    src/wrapper/ZygiskEntryWrapper.cpp \
    src/wrapper/JniEntryWrapper.cpp \
    src/features/Autoplay.cpp \
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
    src/manager/custom_chart/CustomChartImporter.cpp \
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
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/third_party/json/include \
    $(LOCAL_PATH)/third_party/magic_enum/include \
    $(LOCAL_PATH)/third_party \
    $(LOCAL_PATH)/build/generated/lsplt/lsplt/src/main/jni \
    $(LOCAL_PATH)/build/generated/lsplt/lsplt/src/main/jni/include \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/include \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/arch/arm64 \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/common \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/third_party/bsd \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/third_party/lss \
    $(LOCAL_PATH)/third_party/shadowhook/shadowhook/src/main/cpp/third_party/xdl
LOCAL_STATIC_LIBRARIES := libcxx
LOCAL_CONLYFLAGS += -std=c11 -fvisibility=hidden
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384 -Wl,--wrap=dlopen
LOCAL_LDLIBS += -llog -landroid -ldl -lz
include $(BUILD_SHARED_LIBRARY)

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
