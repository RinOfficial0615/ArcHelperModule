from __future__ import annotations

import pathlib
import shutil
import json
import subprocess
import zipfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
WORKSPACE = ROOT.parent
BUILD = ROOT / "build" / "host-tests"
BUILD.mkdir(parents=True, exist_ok=True)

exe = BUILD / "host_tests.exe"
compile_cmd = [
    "g++",
    "-std=c++23",
    "-O2",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-static",
    "-static-libgcc",
    "-static-libstdc++",
    "-I",
    str(ROOT / "src"),
    "-I",
    str(ROOT / "third_party" / "json" / "include"),
    str(ROOT / "tests" / "host_tests.cpp"),
    str(ROOT / "src" / "utils" / "Sha256.cpp"),
    str(ROOT / "src" / "utils" / "ZipArchive.cpp"),
    "-lz",
    "-o",
    str(exe),
]
subprocess.run(compile_cmd, check=True, cwd=ROOT)

runtime_config_exe = BUILD / "runtime_config_host_test.exe"
runtime_config_compile = [
    "g++",
    "-std=c++23",
    "-O2",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-static",
    "-static-libgcc",
    "-static-libstdc++",
    "-DARC_HELPER_HOST_TEST",
    "-I",
    str(ROOT / "tests" / "stubs"),
    "-I",
    str(ROOT / "src"),
    "-I",
    str(ROOT),
    "-I",
    str(ROOT / "third_party" / "json" / "include"),
    str(ROOT / "tests" / "runtime_config_host_test.cpp"),
    str(ROOT / "src" / "config" / "RuntimeConfig.cpp"),
    "-o",
    str(runtime_config_exe),
]
subprocess.run(runtime_config_compile, check=True, cwd=ROOT)
defaults_root = BUILD / "runtime-config-root"
subprocess.run([str(runtime_config_exe), str(defaults_root)], check=True, cwd=ROOT)
print("validated beautified default config generation when config.json is absent")

valid = sorted((WORKSPACE / "ArcCreate" / "arcpkg-samples").glob("*.arcpkg"))
valid += sorted((WORKSPACE / "ArcCreate" / "raw-zip-samples").glob("*.zip"))
if len(valid) < 7:
    raise SystemExit(f"expected at least 7 package fixtures, found {len(valid)}")

# The Strawberry MinGW CRT exposes narrow argv; stage fixtures under ASCII names
# so the native host test remains deterministic on Windows.
staged: list[pathlib.Path] = []
for index, source in enumerate(valid):
    target = BUILD / f"fixture-{index}{source.suffix}"
    shutil.copyfile(source, target)
    staged.append(target)

unsafe = BUILD / "zip-slip.zip"
with zipfile.ZipFile(unsafe, "w", zipfile.ZIP_DEFLATED) as archive:
    archive.writestr("../escape.aff", "AudioOffset:0\n-\ntiming(0,120,4);\n")
short_archive = BUILD / "short.zip"
short_archive.write_bytes(b"PK\x03\x04")

subprocess.run(
    [str(exe), *map(str, staged), "--expect-fail", str(unsafe),
     "--expect-fail", str(short_archive)],
    check=True,
    cwd=ROOT,
)
print(f"validated {len(valid)} real fixtures and two rejected invalid fixtures")

import_root = BUILD / "import-root"
if import_root.exists():
    shutil.rmtree(import_root)
charts_dir = import_root / "charts"
charts_dir.mkdir(parents=True)
(import_root / "config.json").write_text(
    '{"autoplay":false,"customCharts":false broken}', encoding="utf-8"
)
for index, source in enumerate(valid):
    shutil.copyfile(source, charts_dir / f"package-{index}{source.suffix}")

# Minimal raw ZIP with broken metadata: importer must recover from audio + AFF.
with zipfile.ZipFile(charts_dir / "minimal-broken-json.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    archive.writestr("songlist.json", "{broken")
    archive.writestr("base.ogg", b"OggS-host-test")
    archive.writestr("2.aff", "AudioOffset:0\n-\ntiming(0,180,4);\n")

# Broken metadata with two independent folders must still import both songs.
with zipfile.ZipFile(charts_dir / "multi-broken-json.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    archive.writestr("songlist.json", "{broken")
    for song_id, bpm in (("alpha", 150), ("beta", 200)):
        archive.writestr(f"{song_id}/base.ogg", b"OggS-host-test")
        archive.writestr(
            f"{song_id}/2.aff",
            f"AudioOffset:0\n-\ntiming(0,{bpm},4);\n",
        )

importer_exe = BUILD / "importer_host_test.exe"
import_compile = [
    "g++",
    "-std=c++23",
    "-O2",
    "-Wall",
    "-Wextra",
    "-Werror",
    "-static",
    "-static-libgcc",
    "-static-libstdc++",
    "-DARC_HELPER_HOST_TEST",
    "-I",
    str(ROOT / "tests" / "stubs"),
    "-I",
    str(ROOT / "src"),
    "-I",
    str(ROOT),
    "-I",
    str(ROOT / "third_party" / "json" / "include"),
    str(ROOT / "tests" / "importer_host_test.cpp"),
    str(ROOT / "tests" / "asset_virtualizer_stub.cpp"),
    str(ROOT / "src" / "config" / "RuntimeConfig.cpp"),
    str(ROOT / "src" / "features" / "CustomChartManager.cpp"),
    str(ROOT / "src" / "utils" / "Sha256.cpp"),
    str(ROOT / "src" / "utils" / "ZipArchive.cpp"),
    "-lz",
    "-o",
    str(importer_exe),
]
subprocess.run(import_compile, check=True, cwd=ROOT)
subprocess.run([str(importer_exe), str(import_root), "10"], check=True, cwd=ROOT)

report = json.loads((import_root / "import-report.json").read_text(encoding="utf-8"))
statuses = [entry["status"] for entry in report["entries"]]
assert statuses.count("LOADED") == 10, statuses
assert "DEFAULTED_FIELD" in statuses
print("validated multi-package import, broken JSON fallback, defaults, cache, and reports")

# Removing a source package must remove its song and orphaned content-addressed cache.
(charts_dir / "package-6.zip").unlink()
subprocess.run([str(importer_exe), str(import_root), "9"], check=True, cwd=ROOT)
manifest_after_delete = json.loads((import_root / "manifest.json").read_text(encoding="utf-8"))
assert manifest_after_delete["songs"] == 9
cache_dirs = [path for path in (import_root / "cache").iterdir() if path.is_dir()]
assert len(cache_dirs) == 8, cache_dirs
print("validated source deletion and orphan cache cleanup")
