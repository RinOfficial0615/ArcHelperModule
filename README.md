# ArcHelperModule

Language: English | [简体中文](README_CN.md)

Arcaea helper module with autoplay, network controls, and local custom charts for 6.12.11c / 6.13.2f / 6.14.0c / 6.16.2c (arm64). Custom charts start at 6.16.2c.

Provides both Zygisk and JNI entry points. You can embed the built .so into an APK and load it after `libcocos2dcpp.so`, or install it as a Zygisk module directly.

## Requirements

- Android NDK r29+ (the build script picks the newest; fails if ≤ r28)
- Device with Zygisk enabled
- Arcaea 6.12.11c, 6.13.2f, 6.14.0c, or 6.16.2c

## Build

This repo uses a submodule — fetch it first:

```powershell
git submodule update --init --recursive
```

Set `ANDROID_NDK_HOME`, then build:

```powershell
./build.ps1 --rel
```

Artifact: `build/ArcHelperModule.zip`

## Features

| Feature | `config.json` key | Purpose |
|---------|-------------------|---------|
| Autoplay | `autoplay` | Drive arcs & holds, force Pure, suppress effects |
| Network logging | `networkLogger` | Audit HTTP request / response traffic (off by default) |
| Ordinary network block | `networkBlock` | Apply score/world-mode URL rules; mandatory custom-session isolation remains independent |
| SSL pinning bypass | `sslPinningBypass` | Remove SSL pinning on profiles with complete patch offsets (off by default) |
| Custom charts | `customCharts` | Load `.arcpkg` and raw ZIP at startup; force-block all network during local sessions |

## Runtime version detection

Hooks are not installed immediately. `GameVersionManager` probes the game build and waits until the native `appVersion` string matches a supported version. Unknown builds stay in a "detected, not armed" state — wrong offsets are never applied.

## Configuration layout

Runtime directory: `Android/data/<package>/files/ArcHelper/`. The module creates a default `config.json`, scans `charts/` at startup, and writes `manifest.json` plus `import-report.json`.

Configuration and packages are read once per process. A malformed global configuration falls back to all defaults. A raw ZIP needs only audio plus AFF files; absent metadata defaults to `side=1`, `bg=base_conflict`, an official fallback jacket, and deterministic title/BPM values. The first release supports custom difficulty slots `0..3`.

- `src/config/GameProfile.hpp` — per-version function / RTTI / patch offsets
- `src/config/GameStructs.hpp` — game object layouts (padded, compile-time verified)
- `src/config/AutoplayConfig.h` — autoplay behaviour knobs & byte signatures
- `src/config/NetworkBlockConfig.h` — network policy, block rules, byte signatures
- `src/config/ModuleConfig.h` — feature toggles, target package names
- `scope.txt` — module scope

## AI

This project was developed with AI assistance.

## Docs

- Project structure: `docs/project-structure.md`
- Version support: `docs/version-support.md`
- Offsets reference: `docs/6.12.11c-offsets.md`
- 6.16.2c offset evidence: `docs/6.16.2c-offsets_CN.md`
- Device checklist: `DEVICE_TEST_CHECKLIST_CN.md`
