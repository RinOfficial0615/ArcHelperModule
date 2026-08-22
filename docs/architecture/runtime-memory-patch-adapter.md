# Runtime Memory and Patch Adapter

## Boundaries

`src/utils/memory/RuntimeMemory.*` is the checked access boundary for runtime
addresses. It validates non-zero ranges, checked address arithmetic, alignment
for typed access, and complete `/proc/self/maps` permissions before reading or
writing. `RuntimeMemory::Process()` is the production backend; host tests use
an injected `MemoryBackend` so failure paths remain deterministic.

Permission queries reuse a thread-local `/proc/self/maps` snapshot for at most
16 ms, avoiding one full file scan per note field in a gameplay frame. A
successful `mprotect` increments a global generation so every thread refreshes
before its next checked access.

`src/utils/memory/PatchTransaction.*` groups byte patches into one all-or-
rollback operation:

1. Read and compare every expected byte sequence.
2. Reject malformed, overlapping, unmapped, or overflowing descriptors.
3. Save each touched page's original permissions and make only those pages
   writable.
4. Apply replacements and flush the instruction cache.
5. Restore page permissions. If any restore fails, the transaction immediately
   rolls every applied patch back; if that recovery also fails it enters
   `Degraded`, which callers can retry with `Rollback()`.

The transaction owns the original bytes and automatically attempts rollback in
its destructor while it is applied or degraded. Features also retry a degraded
rollback immediately for Autoplay, the song-list digest guards, and SSL pinning
patches.

## Inline hooks

`HookManager` exposes a Register -> Commit flow. Registration validates the
target, signature, executable mapping, duplicate target/handler, and hook
handler before installing anything. `CommitInlineHook` installs the complete
batch and reverses already-installed entries if a later entry fails. A pending
registration is RAII-owned, so an uncommitted registration restores itself.

The AArch64 backend is ShadowHook v2.0.1 (`third_party/shadowhook`, MIT),
which performs instruction relocation and supplies both a callable original
trampoline and an opaque uninstall stub. The adapter is in
`src/utils/memory/ShadowHookAdapter.*`; `InlineHook` keeps the small project-
specific interface used by `HookManager`.

ShadowHook's linker scanner requires `libshadowhook_nothing.so`. The build
therefore compiles `module/shadowhook_nothing.c` as a sibling helper and puts
it next to `zygisk/arm64-v8a.so` in the release package. It cannot be
statically merged into `libarc_helper.so`: ShadowHook calls
`dlopen("libshadowhook_nothing.so")` and uses the resulting independent
`soinfo` constructor event for its layout scan. The helper is linked without
libc/startup objects and stripped to keep the required second ELF small. The
adapter locates that sibling with `dladdr`; the main DSO's linker wrap rewrites
only ShadowHook's basename `dlopen` to that absolute path. ShadowHook itself
remains at the pinned, clean upstream submodule commit, and the helper is still
first loaded after its constructor hook is armed.

LSPlt also remains at its pinned upstream commit. ArcHelper's live-PLT changes
are stored in `patches/lsplt-live-plt.patch` and applied to a disposable
`build/generated/lsplt` copy, so clean clones do not depend on dirty submodule
state.

## Dynamic symbols

`HookManager::RegisterInlineHookSymbol` resolves exported symbols with
`dlopen`/`dlsym` and keeps the library handle alive for the hook lifetime.
AssetVirtualizer uses this path for FMOD `loadBGM`, and the always-on
CxaThrowTracer resolves `__cxa_throw` the same way; the remaining game-private
methods continue to use profile signatures and offsets.

## Verification

- `python tests/run_host_tests.py`
- `python tests/verify_profile.py`
- `./build.ps1 --rebuild --rel`

The release archive was checked to contain `zygisk/arm64-v8a.so`,
`zygisk/libshadowhook_nothing.so`, `module.prop`, and `scope.txt`. Device
smoke testing and hook behavior inside a live game process remain outside this
host/build verification boundary.
