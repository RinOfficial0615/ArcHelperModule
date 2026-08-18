from __future__ import annotations

import hashlib
import pathlib
import struct
import zipfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
SO = ROOT.parent / "6.16.2c" / "libcocos2dcpp.so"
EXPECTED_SHA256 = "ae723637c967e063a5fa0a906325acf2d030b42143613abefd26a11b9002bdcd"

expected = {
    0xBEE260: "e80f19fcfd7b01a9fc6f02a9fa6703a9",
    0x184FBD4: "ff4306d1ea8b00fde923126dfd7b13a9",
    0x745BC4: "ff8302d1fd7b04a9fb2b00f9fa6706a9",
    0x1361DE4: "ff0302d1fd7b04a9f85f05a9f65706a9",
    0x103A07C: "ff8302d1e81b00fdfd7b04a9fc6f05a9",
    0x14A948C: "ffc301d1e81b00fdfd7b04a9f65705a9",
    0xA193CC: "ff0302d1e81b00fdfd7b04a9f85f05a9",
    0xE59C90: "ffc300d1fd7b01a9f44f02a9fd430091",
    0x17E9698: "ff8301d1fd7b02a9f71b00f9f65704a9",
    0x778FD0: "ff0304d1fd7b0fa9fdc30391a20f39a9",
    # 6.16.2c songlist data loader: AAssetManager_open BL at 0x142CFAC,
    # whose return address is the exact caller 0x142CFB0.
    0x142CFAC: "79041494f40300aae00700b4e00314aa",
    # Song-list builder. It filters candidate songs by the exact current
    # difficulty and stores each result as a 16-byte (song, difficulty) pair.
    0x12638FC: "ff8303d1fd7b08a9fc6f09a9fa670aa9",
    # Final difficulty availability predicate used by song cells and selection.
    0xE162C8: "ff8301d1fd7b02a9f85f03a9f65704a9",
    # Song-level unlock mask predicate shared by song cells and play entry.
    0xD988F4: "ffc301d1fd7b04a9f65705a9f44f06a9",
    # Remote-pack predicate. 1 + Beyond (class 3) selects img/download.png.
    0x121E9FC: "fd7bbda9f65701a9f44f02a9fd030091",
    # Chart path. Beyond/remote uses writable {id}_{n} pack, not songs/{id}/n.aff.
    0xA74680: "ff8303d1fd7b0ba9f6570ca9f44f0da9",
    # Runtime song registry lookup used to recover the parsed custom-song
    # object after the official unlock filter omits unknown server IDs.
    0xCADFA4: "fd7bbea9f30b00f9fd030091f30300aa",
    # Separate integrity/existence probe; it must not be accepted as loader.
    0xB5E240: "d43f3794600000b4d63f379420008052081740f9a9835ff81f0109eb41010054",
}
patches = {
    0xBEE748: 0x11019148,
    0xBEE800: 0x11019148,
    0xBEE850: 0x11032148,
    # Songlist-only digest failure branches. Runtime patching replaces these
    # with NOP while leaving packlist and unlocks validation intact.
    0x100F814: 0x540013A1,
    0x100F830: 0x350012C0,
}


def elf_dynamic_symbols(elf: bytes) -> dict[str, tuple[int, int, int]]:
    """Return dynsym name -> (value, size, binding/type) for ELF64 LE files."""
    assert elf[:4] == b"\x7fELF" and elf[4] == 2 and elf[5] == 1, "expected ELF64 little-endian"
    section_header_offset = struct.unpack_from("<Q", elf, 0x28)[0]
    section_header_size = struct.unpack_from("<H", elf, 0x3A)[0]
    section_count = struct.unpack_from("<H", elf, 0x3C)[0]
    section_name_index = struct.unpack_from("<H", elf, 0x3E)[0]
    section_headers = [
        struct.unpack_from("<IIQQQQIIQQ", elf, section_header_offset + index * section_header_size)
        for index in range(section_count)
    ]
    name_header = section_headers[section_name_index]
    section_names = elf[name_header[4] : name_header[4] + name_header[5]]

    def section_name(header: tuple[int, ...]) -> bytes:
        start = header[0]
        return section_names[start : section_names.find(b"\0", start)]

    dynsym = next(header for header in section_headers if section_name(header) == b".dynsym")
    dynstr = next(header for header in section_headers if section_name(header) == b".dynstr")
    strings = elf[dynstr[4] : dynstr[4] + dynstr[5]]
    entry_size = dynsym[9] or 24
    symbols = {}
    for offset in range(dynsym[4], dynsym[4] + dynsym[5], entry_size):
        st_name, st_info, _st_other, st_shndx, st_value, st_size = struct.unpack_from(
            "<IBBHQQ", elf, offset
        )
        end = strings.find(b"\0", st_name)
        if st_name and end >= 0:
            symbols[strings[st_name:end].decode("utf-8")] = (st_value, st_size, st_info)
    return symbols


data = SO.read_bytes()
actual_sha256 = hashlib.sha256(data).hexdigest()
assert actual_sha256 == EXPECTED_SHA256, (actual_sha256, EXPECTED_SHA256)
symbols = elf_dynamic_symbols(data)
setter_symbol = symbols.get("Java_low_moe_AppActivity_setAppVersion")
assert setter_symbol is not None, "setAppVersion ELF dynamic symbol missing"
assert setter_symbol[2] & 0x0F == 2 and setter_symbol[2] >> 4 == 1, setter_symbol
assert setter_symbol[1] > 0, setter_symbol
cxa_throw_symbol = symbols.get("__cxa_throw")
assert cxa_throw_symbol is not None, "__cxa_throw ELF dynamic symbol missing"
assert cxa_throw_symbol[2] & 0x0F == 2 and cxa_throw_symbol[2] >> 4 == 1, cxa_throw_symbol
assert cxa_throw_symbol[1] > 0, cxa_throw_symbol
cxa_throw_signature = bytes.fromhex("3f2303d5fd7bbca9f70b00f9f65702a9")
assert data[
    cxa_throw_symbol[0] : cxa_throw_symbol[0] + len(cxa_throw_signature)
] == cxa_throw_signature
for offset, hex_bytes in expected.items():
    value = bytes.fromhex(hex_bytes)
    assert data[offset : offset + len(value)] == value, hex(offset)
for offset, instruction in patches.items():
    assert int.from_bytes(data[offset : offset + 4], "little") == instruction, hex(offset)

print(f"verified profile against {SO.name} sha256={actual_sha256}")
print(
    "verified setAppVersion dynsym "
    f"value=0x{setter_symbol[0]:x} size={setter_symbol[1]}"
)
print(
    "verified __cxa_throw dynsym "
    f"value=0x{cxa_throw_symbol[0]:x} size={cxa_throw_symbol[1]}"
)

APK = ROOT / ".tmp" / "arcaea_6.16.2c_arc_helper_diag.apk"
if APK.is_file():
    with zipfile.ZipFile(APK) as archive:
        provider = archive.read("lib/arm64-v8a/libfmodProvider.so")
    provider_sha256 = hashlib.sha256(provider).hexdigest()
    assert provider_sha256 == "1f3907b13d3ce3ef6b3d25a92edc971d5763d0b3220a9a99597424481d06a291"
    provider_symbols = elf_dynamic_symbols(provider)
    fmod_symbol = provider_symbols.get("_ZN24AudioProviderFMODAndroid7loadBGMEPKci")
    assert fmod_symbol is not None, "FMOD loadBGM ELF dynamic symbol missing"
    assert fmod_symbol[2] & 0x0F == 2 and fmod_symbol[2] >> 4 == 1, fmod_symbol
    assert fmod_symbol[1] > 0, fmod_symbol
    print(f"verified FMOD provider from {APK.name} sha256={provider_sha256}")
    print(f"verified FMOD loadBGM dynsym value=0x{fmod_symbol[0]:x} size={fmod_symbol[1]}")
