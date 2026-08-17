#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "manager/HookManager.hpp"

namespace {

int g_install_calls = 0;
int g_restore_calls = 0;
int g_fail_install_call = 0;
int g_fail_restore_count = 0;

alignas(16) std::array<uint8_t, 16> g_target_a{};
alignas(16) std::array<uint8_t, 16> g_target_b{};
alignas(16) std::array<uint8_t, 16> g_target_c{};
alignas(16) std::array<uint8_t, 16> g_target_d{};
alignas(16) std::array<uint8_t, 16> g_target_e{};
alignas(16) std::array<uint8_t, 16> g_target_f{};

void HookA() {}
void HookB() {}
void HookC() {}
void HookD() {}
void HookE() {}
void HookF() {}

std::array<uint8_t, 16> MakeSignature(uint8_t seed) {
    std::array<uint8_t, 16> signature{};
    for (size_t i = 0; i < signature.size(); ++i) {
        signature[i] = static_cast<uint8_t>(seed + i);
    }
    return signature;
}

} // namespace

namespace arc_helper {

GameManager &GameManager::Instance() {
    static GameManager manager;
    return manager;
}

uintptr_t GameManager::GetGameLibBase() const {
    return 0;
}

uintptr_t GameManager::GetOrFindGameLibBase() {
    return 0;
}

} // namespace arc_helper

namespace arc_helper::mem {

uintptr_t AddressResolver::ResolveBySignature(uintptr_t,
                                              const uint8_t *,
                                              size_t,
                                              std::string_view) const {
    return 0;
}

bool ProcMaps::IsReadable(uintptr_t addr, size_t len) {
    return addr != 0 && len <= 16;
}

bool ProcMaps::IsExecutable(uintptr_t addr) {
    return addr != 0;
}

bool InlineHook::InstallA64(uintptr_t target, void *, void **orig_fn_out, void **stub_out) {
    ++g_install_calls;
    if (g_fail_install_call == g_install_calls) return false;
    *orig_fn_out = reinterpret_cast<void *>(target + 1);
    *stub_out = reinterpret_cast<void *>(target + 2);
    return true;
}

bool InlineHook::RestoreA64(uintptr_t, void *, void *) {
    ++g_restore_calls;
    if (g_fail_restore_count > 0) {
        --g_fail_restore_count;
        return false;
    }
    return true;
}

} // namespace arc_helper::mem

int main() {
    using Registration = arc_helper::HookManager::InlineHookRegistration;
    auto &manager = arc_helper::HookManager::Instance();

    const auto sig_a = MakeSignature(0x10);
    const auto sig_b = MakeSignature(0x30);
    g_target_a = sig_a;
    g_target_b = sig_b;

    std::array<Registration, 2> success = {
        manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_a.data()),
                                           sig_a,
                                           HookA,
                                           "hook-a"),
        manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_b.data()),
                                           sig_b,
                                           HookB,
                                           "hook-b"),
    };
    assert(success[0] && success[1]);
    assert(g_install_calls == 0);
    assert(manager.CommitInlineHook(std::span<Registration>(success)));
    assert(g_install_calls == 2);
    assert(g_restore_calls == 0);
    assert(manager.HasOriginalForHook(reinterpret_cast<void *>(HookA)));
    assert(manager.HasOriginalForHook(reinterpret_cast<void *>(HookB)));

    const auto sig_e = MakeSignature(0x90);
    const auto sig_f = MakeSignature(0xB0);
    g_target_e = sig_e;
    g_target_f = sig_f;
    g_fail_install_call = g_install_calls + 2;
    {
        std::array<Registration, 2> failure = {
            manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_e.data()),
                                               sig_e, HookE, "hook-e"),
            manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_f.data()),
                                               sig_f, HookF, "hook-f"),
        };
        assert(failure[0] && failure[1]);
        assert(!manager.CommitInlineHook(std::span<Registration>(failure)));
    }
    assert(!manager.HasOriginalForHook(reinterpret_cast<void *>(HookE)));
    assert(!manager.HasOriginalForHook(reinterpret_cast<void *>(HookF)));
    auto hook_e_retry = manager.RegisterInlineHookAbsolute(
        reinterpret_cast<uintptr_t>(g_target_e.data()), sig_e, HookE, "hook-e-retry");
    assert(hook_e_retry);

    const auto sig_c = MakeSignature(0x50);
    const auto sig_d = MakeSignature(0x70);
    g_target_c = sig_c;
    g_target_d = sig_d;
    g_fail_install_call = g_install_calls + 2;
    g_fail_restore_count = 1;
    const int restore_before_transient_failure = g_restore_calls;

    {
        std::array<Registration, 2> failure = {
            manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_c.data()),
                                               sig_c,
                                               HookC,
                                               "hook-c"),
            manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_d.data()),
                                               sig_d,
                                               HookD,
                                               "hook-d"),
        };
        assert(failure[0] && failure[1]);
        assert(!manager.CommitInlineHook(std::span<Registration>(failure)));
        assert(g_restore_calls == restore_before_transient_failure + 1);
        // The native hook is still active after the first failed unhook, so
        // the manager must retain its original trampoline until recovery.
        assert(manager.HasOriginalForHook(reinterpret_cast<void *>(HookC)));
    }

    assert(g_restore_calls == restore_before_transient_failure + 2);
    assert(!manager.HasOriginalForHook(reinterpret_cast<void *>(HookD)));

    g_fail_install_call = g_install_calls + 2;
    g_fail_restore_count = 2;
    {
        std::array<Registration, 2> failure = {
            manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_c.data()),
                                               sig_c,
                                               HookC,
                                               "hook-c-persistent"),
            manager.RegisterInlineHookAbsolute(reinterpret_cast<uintptr_t>(g_target_d.data()),
                                               sig_d,
                                               HookD,
                                               "hook-d-persistent"),
        };
        assert(failure[0] && failure[1]);
        assert(!manager.CommitInlineHook(std::span<Registration>(failure)));
    }
    assert(manager.HasOriginalForHook(reinterpret_cast<void *>(HookC)));

    g_fail_restore_count = 0;
    {
        auto recovered = manager.RegisterInlineHookAbsolute(
            reinterpret_cast<uintptr_t>(g_target_c.data()), sig_c, HookC, "hook-c-retry");
        assert(recovered);
    }
    assert(!manager.HasOriginalForHook(reinterpret_cast<void *>(HookC)));
    return 0;
}
