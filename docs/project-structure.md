# Project Structure

Language: English | [简体中文](project-structure_CN.md)

## Source Tree

- `src/wrapper/ZygiskEntryWrapper.cpp` — Zygisk entry, hooks `Runtime.nativeLoad`
- `src/wrapper/JniEntryWrapper.cpp` — JNI entry (`JNI_OnLoad`), for loading after `libcocos2dcpp.so`
- `src/wrapper/WrapperCommon.hpp` — shared init logic, pkg check, feature bootstrap
- `src/manager/GameManager.{hpp,cpp}` — caches `libcocos2dcpp.so` base
- `src/manager/GameVersionManager.{hpp,cpp}` — detects game build, activates matching profile
- `src/manager/HookManager.{hpp,cpp}` — inline hook helper (`InstallInlineHook`, `CALL_ORIG`, restore)
- `src/manager/NetworkManager.{hpp,cpp}` — network hooks, handler dispatch
- `src/manager/network/NetworkHandler.hpp` — handler API (`HandlerArgs`, body views, phases)
- `src/features/Feature.hpp` — feature base, lib base caching
- `src/features/Autoplay.{hpp,cpp}` — autoplay hooks, synthetic touch, patch application
- `src/features/NetworkLogger.{hpp,cpp}` — high-priority request/response audit
- `src/features/NetworkBlock.{hpp,cpp}` — low-priority URL block policy
- `src/features/SslPinningBypass.{hpp,cpp}` — SSL pin removal (two byte-patches)
- `src/features/CustomChartManager.{hpp,cpp}` — startup `.arcpkg`/raw ZIP import, songlist merge, cache and reports
- `src/features/AssetVirtualizer.{hpp,cpp}` — virtual songlist/custom assets and official-default asset redirects
- `src/features/CustomSession.{hpp,cpp}` — custom-chart session state used for mandatory full network isolation
- `src/GameTypes.hpp` — thin wrappers for `Gameplay`, `LogicArcNote`, `LogicHoldNote` etc
- `src/config/RuntimeConfig.{hpp,cpp}` — startup-only runtime configuration from the fixed data directory
- `src/utils/MemoryUtils.hpp` — umbrella include for memory tools
- `src/utils/memory/*.hpp|*.cpp` — `ProcMaps`, `AddressResolver`, `Patcher`, `InlineHook`
- `src/utils/Log.h` — `ARC_LOGI` / `ARC_LOGE` macros
- `third_party/json/` — nlohmann/json submodule used for parsing and serialization
- `src/utils/Sha256.{hpp,cpp}` — content hashing and stable-ID support
- `src/utils/ZipArchive.{hpp,cpp}` — ZIP reader with path, size, CRC, and ratio validation
- `src/config/GameStructs.hpp` — version-templated layout structs (explicit padding, `offsetof`-verified)
- `src/config/GameProfile.hpp` — per-version function/RTTI/patch offsets
- `src/config/AutoplayConfig.h` — autoplay behaviour knobs & byte signatures
- `src/config/NetworkBlockConfig.h` — network policy, block rules, byte signatures
- `src/config/ModuleConfig.h` — feature toggles, target package names
- `module/` — Magisk/Zygisk packaging metadata, scope, and configuration example
- `third_party/libcxx/` — Android libc++ submodule
- `third_party/lsplt/` — LSPlt submodule
- `third_party/zygisk.hpp` — Zygisk API header

## Initialization Flow

1. Wrapper detects `libcocos2dcpp.so` → caches base in `GameManager`.
2. Zygisk path validates package; JNI path skips package check.
3. `GameVersionManager` identifies the candidate build and waits for the real `appVersion`.
4. Once the version is confirmed, the wrapper callback instantiates features and installs network-isolation hooks.
5. Only when the profile advertises custom-chart support and network isolation is available does the module scan packages, import them, and install asset virtualization.
6. Features install through `HookManager` according to the startup configuration and active profile; configuration is not hot-reloaded in the current process.
