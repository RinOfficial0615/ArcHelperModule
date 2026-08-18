# Repository Guidelines

## Project Structure & Module Organization

First-party C++23 code lives in `src/`. Entry points are in `src/wrapper/`, runtime coordination in `src/manager/`, user-facing behavior in `src/features/`, version profiles and game layouts in `src/game/`, feature knobs in `src/config/`, and reusable helpers in `src/utils/`. Host tests live in `tests/`, packaging files in `module/`, and notes in `docs/`. Dependencies are submodules under `third_party/`; avoid unrelated edits or formatting churn there.

## Reuse Before Implementing

**Do not reinvent the wheel.** Search `src/utils/`, `third_party/`, and the Internet first, then reuse established parsing, hooking, hashing, archive, and memory components.

## Modern C++ and Design Rules

- Always try the newest C++ features supported by the current NDK Clang++ before writing older equivalent syntax; prefer concepts, ranges, `std::expected`-style status types where available, and standard-library vocabulary types when they improve clarity.
- Do not over-design. Apply the deletion test and introduce a new module, seam, adapter, or abstraction only when it removes real duplication or gives callers a smaller, deeper interface.
- Keep version and toolchain constraints explicit. Code should target the repository's C++23 NDK Clang++ baseline rather than preserving compatibility with unrelated host compilers.

## Build, Test, and Development Commands

- `git submodule update --init --recursive` initializes the pinned C++ runtime dependency.
- `./build.ps1` creates a debug arm64 build with Android NDK r29 or newer.
- `./build.ps1 --rel` strips and packages `build/ArcHelperModule.zip`.
- `./build.ps1 --rebuild --rel` removes prior output and forces a clean release build.
- `python tests/run_host_tests.py` compiles host tests with the newest installed NDK `clang++` and validates JSON, hashing, ZIP safety, configuration, logging, hooks, and chart importing. It expects sample packages in the sibling `ArcCreate` repository; `g++` compatibility is not a test target.
- `python tests/verify_profile.py` checks the 6.16.2c offsets against `../6.16.2c/libcocos2dcpp.so`, including its exact SHA-256.

## Coding Style & Naming Conventions

Use four-space indentation and follow the surrounding C++ style. Types and public methods use `PascalCase`; local variables and private fields use `snake_case`, with private members ending in `_`; constants use `kPascalCase`. Pair implementation files as `Name.hpp` and `Name.cpp`, keep code in `namespace arc_helper`, and prefer bounded parsing plus explicit validation for untrusted runtime data. The build disables exceptions and RTTI, so use status values and error strings instead.

## Testing Guidelines

Add focused host coverage for portable logic, naming new files `*_host_test.cpp`. Treat warnings as failures (`-Wall -Wextra -Werror`). For hook profiles or layout changes, verify signatures, offsets, struct assertions, the matching game binary hash, and an arm64 release build. No coverage percentage is enforced; regressions in parsing, archive safety, configuration defaults, and unsupported-version behavior require tests.

## Commit & Pull Request Guidelines

History favors short, imperative subjects, sometimes with `feat:` or `fix:` prefixes. Keep each commit scoped. Pull requests should describe affected game versions and features, list commands run, link relevant issues, and include device logs or screenshots for runtime-visible behavior. Document new offsets and never commit game binaries, generated `build/` output, or personal `config.json` files.
