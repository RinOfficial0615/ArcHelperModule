#include "manager/GameVersionManager.hpp"

#include <array>

#include "config/ModuleConfig.h"
#include "utils/Log.h"
#include "utils/MemoryUtils.hpp"
#include "utils/memory/RuntimeMemory.hpp"

namespace arc_helper {
namespace {

constexpr size_t kMaxVersionStringLen = 64;

inline constexpr std::array<uint8_t, 16> kSig_SetAppVersion = {
    0xFF, 0x43, 0x02, 0xD1,
    0xFD, 0x7B, 0x04, 0xA9,
    0xF9, 0x2B, 0x00, 0xF9,
    0xF8, 0x5F, 0x06, 0xA9,
};

void ClearPendingJniException(JNIEnv *env, const char *where) {
    if (!env || !env->ExceptionCheck()) return;
    ARC_LOGE("GameVersionManager: JNI exception at %s", where ? where : "unknown");
    env->ExceptionClear();
}

std::string CopyJniString(JNIEnv *env, jstring value) {
    if (!env || !value) return {};

    const char *utf8 = env->GetStringUTFChars(value, nullptr);
    if (!utf8) {
        ClearPendingJniException(env, "GetStringUTFChars(setAppVersion)");
        return {};
    }

    std::string copy(utf8);
    env->ReleaseStringUTFChars(value, utf8);
    return copy;
}

} // namespace

GameVersionManager &GameVersionManager::Instance() {
    static GameVersionManager manager;
    return manager;
}

void GameVersionManager::SetResolvedCallback(ResolvedCallback callback) {
    std::scoped_lock lock(mutex_);
    resolved_callback_ = callback;
    FireResolvedCallback();
}

bool GameVersionManager::IsResolved() const {
    std::scoped_lock lock(mutex_);
    return active_profile_ != nullptr;
}

const cfg::GameProfile *GameVersionManager::GetActiveProfile() const {
    std::scoped_lock lock(mutex_);
    return active_profile_;
}

cfg::GameVersionId GameVersionManager::GetActiveVersionId() const {
    std::scoped_lock lock(mutex_);
    return active_profile_ ? active_profile_->id : cfg::GameVersionId::kUnknown;
}

std::string GameVersionManager::GetResolvedVersionString() const {
    std::scoped_lock lock(mutex_);
    return resolved_version_string_;
}

bool GameVersionManager::EnsureLibBase() {
    std::scoped_lock lock(mutex_);
    if (lib_base_) return true;
    if (!hook_manager_.EnsureReady()) return false;

    lib_base_ = hook_manager_.GetLibBase();
    return lib_base_ != 0;
}

std::string GameVersionManager::ReadLibcxxString(uintptr_t string_addr) const {
    if (!string_addr || !mem::ProcMaps::IsReadable(string_addr, 1)) return {};

    const auto first_result = mem::RuntimeMemory::Process().Read<uint8_t>(string_addr);
    if (!first_result) return {};
    const uint8_t first = *first_result;
    if ((first & 1u) == 0) {
        const size_t size = static_cast<size_t>(first >> 1);
        if (size == 0) return {};
        if (size > 22 || string_addr == UINTPTR_MAX ||
            !mem::ProcMaps::IsReadable(string_addr + 1, size)) return {};
        return std::string(reinterpret_cast<const char *>(string_addr + 1), size);
    }

    if (string_addr > UINTPTR_MAX - 16 ||
        !mem::ProcMaps::IsReadable(string_addr + 8, sizeof(size_t)) ||
        !mem::ProcMaps::IsReadable(string_addr + 16, sizeof(uintptr_t))) {
        return {};
    }
    const auto size_result = mem::RuntimeMemory::Process().Read<size_t>(string_addr + 8);
    const auto data_result = mem::RuntimeMemory::Process().Read<uintptr_t>(string_addr + 16);
    if (!size_result || !data_result) return {};
    const size_t size = *size_result;
    const uintptr_t data = *data_result;
    if (!data || size == 0 || size > kMaxVersionStringLen) return {};
    if (!mem::ProcMaps::IsReadable(data, size)) return {};
    return std::string(reinterpret_cast<const char *>(data), size);
}

std::string GameVersionManager::ReadAppVersionString(const cfg::GameProfile &profile) const {
    if (!lib_base_ ||
        profile.version_probe.app_version_string > UINTPTR_MAX - lib_base_) return {};
    return ReadLibcxxString(lib_base_ + profile.version_probe.app_version_string);
}

bool GameVersionManager::TryResolveFromString(const char *version_string) {
    std::scoped_lock lock(mutex_);
    if (!version_string || version_string[0] == '\0') return false;

    const cfg::GameProfile *profile = cfg::FindGameProfileByVersionString(version_string);
    if (!profile) {
        ARC_LOGE("GameVersionManager: unsupported appVersion '%s'", version_string);
        return false;
    }

    if (active_profile_ == profile && resolved_version_string_ == version_string) {
        FireResolvedCallback();
        return true;
    }

    active_profile_ = profile;
    resolved_version_string_ = version_string;
    ARC_LOGI("GameVersionManager: resolved version %s", resolved_version_string_.c_str());
    FireResolvedCallback();
    return true;
}

bool GameVersionManager::TryResolveFromGlobal(const cfg::GameProfile &profile) {
    std::scoped_lock lock(mutex_);
    const std::string version = ReadAppVersionString(profile);
    return TryResolveFromString(version.c_str());
}

bool GameVersionManager::EnsureInstalled() {
    std::scoped_lock lock(mutex_);
    if (IsResolved()) {
        FireResolvedCallback();
        return true;
    }
    if (!EnsureLibBase()) return false;

    for (const auto &profile : cfg::kSupportedGameProfiles) {
        if (TryResolveFromGlobal(profile)) return true;
    }
    if (hook_installed_) return false;

    auto registration = hook_manager_.RegisterInlineHookSymbol(
        resolved_setter_addr_,
        cfg::module::kLibName,
        "Java_low_moe_AppActivity_setAppVersion",
        kSig_SetAppVersion,
        SetAppVersionHook,
        "Java_low_moe_AppActivity_setAppVersion");
    hook_installed_ = registration && hook_manager_.CommitInlineHook(registration);
    if (!hook_installed_) {
        ARC_LOGE("GameVersionManager: failed to install setAppVersion hook");
        return false;
    }

    for (const auto &profile : cfg::kSupportedGameProfiles) {
        if (TryResolveFromGlobal(profile)) return true;
    }
    return false;
}

void GameVersionManager::OnSetAppVersion(JNIEnv *env, jobject receiver, jstring version_string) {
    const std::string version_copy = CopyJniString(env, version_string);
    CALL_ORIG(SetAppVersionHook, env, receiver, version_string);

    if (!version_copy.empty()) {
        TryResolveFromString(version_copy.c_str());
        return;
    }
    for (const auto &profile : cfg::kSupportedGameProfiles) {
        if (TryResolveFromGlobal(profile)) return;
    }
}

void GameVersionManager::FireResolvedCallback() {
    std::scoped_lock lock(mutex_);
    if (!active_profile_ || !resolved_callback_ || resolved_callback_fired_) return;
    resolved_callback_fired_ = true;
    resolved_callback_();
}

void GameVersionManager::SetAppVersionHook(JNIEnv *env, jobject receiver, jstring version_string) {
    Instance().OnSetAppVersion(env, receiver, version_string);
}

} // namespace arc_helper
