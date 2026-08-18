# Project Structure

Language: English | [简体中文](project-structure_CN.md)

## Source Tree

- `src/wrapper/ZygiskEntryWrapper.cpp` — Zygisk entry, hooks `Runtime.nativeLoad`
- `src/wrapper/JniEntryWrapper.cpp` — JNI entry (`JNI_OnLoad`), for loading after `libcocos2dcpp.so`
- `src/wrapper/WrapperCommon.hpp` — shared init logic, pkg check, feature bootstrap
- `src/manager/GameManager.{hpp,cpp}` — caches `libcocos2dcpp.so` base
- `src/manager/GameVersionManager.{hpp,cpp}` — detects game build, activates matching profile
- `src/manager/HookManager.{hpp,cpp}` — transactional inline hook helper (`RegisterInlineHook`, `CommitInlineHook`, `CALL_ORIG`)
- `src/manager/ConfigManager.{hpp,cpp}` — schema-free JSON loading, typed field registration, normalization, and atomic save
- `src/manager/FeatureManager.{hpp,cpp}` — explicit Feature creation and installation order
- `src/manager/NetworkManager.{hpp,cpp}` — network hooks, handler dispatch
- `src/manager/network/NetworkHandler.hpp` plus `NetworkHandlerSnapshot.{hpp,cpp}` — handler phases, immutable order, and bounded views
- `src/features/Feature.hpp` — feature name and typed configuration helpers (`AH_CFG`)
- `src/features/Logging.{hpp,cpp}` — nested logcat/file sink configuration
- `src/features/Autoplay.{hpp,cpp}` — autoplay hooks, synthetic touch, patch application
- `src/features/NetworkLogger.{hpp,cpp}` — high-priority request/response audit
- `src/features/NetworkBlock.{hpp,cpp}` — low-priority URL block policy
- `src/features/SslPinningBypass.{hpp,cpp}` — SSL pin removal (two byte-patches)
- `src/manager/CustomChartManager.{hpp,cpp}` — publishes and reads the immutable custom-chart snapshot
- `src/manager/custom_chart/CustomChartImporter.{hpp,cpp}` — `.arcpkg`/raw ZIP parsing and bounded cache extraction
- `src/manager/custom_chart/CustomChartAssetIndex.{hpp,cpp}` — canonical logical asset paths and APK-era aliases
- `src/manager/custom_chart/CustomChartSnapshot.{hpp,cpp}` — immutable songlist model and pure official-songlist merge
- `src/manager/custom_chart/CustomChartReportWriter.{hpp,cpp}` — manifest/report writes and orphan-cache commit gate
- `src/manager/custom_chart/CustomChartGameplaySession.{hpp,cpp}` — atomic custom-chart play window; mapped `.aff` starts it, a `kJacketAssetName` read ends it
- `src/features/CustomCharts.{hpp,cpp}` — custom-chart feature lifecycle and AssetVirtualizer installation
- `src/features/AssetVirtualizer.{hpp,cpp}` — virtual songlist/custom assets and official-default asset redirects
- `src/game/GameTypes.hpp` — thin wrappers for `Gameplay`, `LogicArcNote`, `LogicHoldNote` etc
- `src/utils/MemoryUtils.hpp` — umbrella include for memory tools
- `src/utils/memory/*.hpp|*.cpp` — `ProcMaps`, `AddressResolver`, `RuntimeMemory`, `PatchTransaction`, `InlineHook`, `ShadowHookAdapter`
- `src/utils/Log.{h,cpp}` — source-aware `ARC_LOGD/I/W/E`, logcat/file sinks, truncation and rotation
- `third_party/json/` — nlohmann/json submodule used for parsing and serialization
- `src/utils/Sha256.{hpp,cpp}` — content hashing and stable-ID support
- `src/utils/ZipArchive.{hpp,cpp}` — ZIP reader with path, size, CRC, and ratio validation
- `src/game/GameStructs.hpp` — version-templated layout structs (explicit padding, `offsetof`-verified)
- `src/game/GameProfile.hpp` — per-version function/RTTI/patch offsets
- `src/config/AutoplayConfig.h` — autoplay behaviour knobs & byte signatures
- `src/config/NetworkBlockConfig.h` — network policy, block rules, byte signatures
- `src/config/CustomChartConfig.h` — asset aliases, parser bounds, layout guards, and hook signatures
- `src/config/ModuleConfig.h` — module identity and target library names
- `module/` — Magisk/Zygisk packaging metadata, scope, and configuration example
- `third_party/libcxx/` — Android libc++ submodule
- `third_party/lsplt/` — LSPlt submodule
- `third_party/magic_enum/` — compile-time enum reflection submodule
- `third_party/shadowhook/` — ShadowHook v2.0.1 submodule; `patches/lsplt-live-plt.patch` is staged during builds
- `module/shadowhook_nothing.c` — required independent ShadowHook linker-scan helper ELF
- `third_party/zygisk.hpp` — Zygisk API header

## Initialization Flow

1. The wrapper determines the ArcHelper root, then `ConfigManager` loads raw JSON.
2. `FeatureManager::CreateAll()` creates `Logging` first and then every Feature, including disabled ones, before the normalized JSON is atomically saved.
3. `GameVersionManager` reads the version global or dynamically hooks the exported `setAppVersion` symbol, then waits for the real version string.
4. `FeatureManager::InstallAll(profile)` installs Features in a fixed order; constructors never install hooks.
5. Custom charts import only after fixed network isolation is installed and the active profile advertises support.
