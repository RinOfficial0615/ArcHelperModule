from __future__ import annotations

import hashlib
import pathlib


ROOT = pathlib.Path(__file__).resolve().parents[1]
SO = ROOT.parent / "6.16.2c" / "libcocos2dcpp.so"
EXPECTED_SHA256 = "ae723637c967e063a5fa0a906325acf2d030b42143613abefd26a11b9002bdcd"

expected = {
    0x896058: "ff4302d1fd7b04a9f92b00f9f85f06a9",
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
    # Separate integrity/existence probe; it must not be accepted as loader.
    0xB5E240: "d43f3794600000b4d63f379420008052081740f9a9835ff81f0109eb41010054",
}
patches = {
    0xBEE748: 0x11019148,
    0xBEE800: 0x11019148,
    0xBEE850: 0x11032148,
}

data = SO.read_bytes()
actual_sha256 = hashlib.sha256(data).hexdigest()
assert actual_sha256 == EXPECTED_SHA256, (actual_sha256, EXPECTED_SHA256)
for offset, hex_bytes in expected.items():
    value = bytes.fromhex(hex_bytes)
    assert data[offset : offset + len(value)] == value, hex(offset)
for offset, instruction in patches.items():
    assert int.from_bytes(data[offset : offset + 4], "little") == instruction, hex(offset)

print(f"verified profile against {SO.name} sha256={actual_sha256}")
