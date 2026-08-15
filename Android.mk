LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := arc_helper
LOCAL_SRC_FILES := \
    src/wrapper/ZygiskEntryWrapper.cpp \
    src/wrapper/JniEntryWrapper.cpp \
    src/features/Autoplay.cpp \
    src/features/NetworkLogger.cpp \
    src/features/NetworkBlock.cpp \
    src/features/SslPinningBypass.cpp \
    src/features/CustomSession.cpp \
    src/features/CustomChartManager.cpp \
    src/features/AssetVirtualizer.cpp \
    src/config/RuntimeConfig.cpp \
    src/manager/GameManager.cpp \
    src/manager/GameVersionManager.cpp \
    src/manager/NetworkManager.cpp \
    src/manager/HookManager.cpp \
    src/utils/memory/ProcMaps.cpp \
    src/utils/memory/AddressResolver.cpp \
    src/utils/memory/Patcher.cpp \
    src/utils/memory/InlineHook.cpp \
    src/utils/memory/ExecUtils.cpp \
    src/utils/Sha256.cpp \
    src/utils/ZipArchive.cpp \
    third_party/lsplt/lsplt/src/main/jni/lsplt.cc \
    third_party/lsplt/lsplt/src/main/jni/elf_util.cc
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/third_party/json/include \
    $(LOCAL_PATH)/third_party \
    $(LOCAL_PATH)/third_party/lsplt/lsplt/src/main/jni \
    $(LOCAL_PATH)/third_party/lsplt/lsplt/src/main/jni/include
LOCAL_STATIC_LIBRARIES := libcxx
LOCAL_LDLIBS += -llog -landroid -lz
include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/third_party/libcxx/Android.mk
