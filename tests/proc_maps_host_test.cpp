#include <cassert>
#include <cstdint>
#include <string_view>

#include "utils/memory/ProcMaps.hpp"

namespace {

constexpr std::string_view kSoname = "libcocos2dcpp.so";

// Android 16 JNI failure: remapped GNU_RELRO sits at a lower VA than RX, so
// min(start - offset) yields a bias that is not the executable PT_LOAD.
constexpr std::string_view kAndroid16RelroBelowRx = R"(
73298ec000-7329a00000 r--p 0193c000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
732ead7000-732f36d000 r-xp 00000000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
732f36d000-732f36e000 rwxp 00896000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
732f36e000-7330414000 r-xp 00897000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
733052e000-733056f000 rw-p 01a4f000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
)";

// Contiguous load (successful JNI session). RELRO is above RX and shares bias.
constexpr std::string_view kContiguousLoad = R"(
7329ae6000-732a22b000 r-xp 00000000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
732a22b000-732a22c000 rwxp 00745000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
732a6d4000-732a6d5000 rwxp 00bee000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
73302a6000-73303ba000 r--p 0193c000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
732b53d000-732b57e000 rw-p 01a4f000 fe:28 1015073                        /data/app/~~x==/moe.low.arc-y==/lib/arm64/libcocos2dcpp.so
)";

constexpr std::string_view kDeletedSuffix = R"(
72aa000000-72aa100000 r-xp 00000000 fe:28 1                              /data/app/lib/arm64/libcocos2dcpp.so (deleted)
)";

} // namespace

int main() {
    using arc_helper::mem::ProcMaps;

    const uintptr_t remapped = ProcMaps::FindLibraryBaseFromMaps(kAndroid16RelroBelowRx, kSoname);
    assert(remapped == 0x732ead7000ull);
    assert(remapped != 0x7327fb0000ull);
    constexpr uintptr_t kProcessLogicNotes = 0xbee260ull;
    const uintptr_t remapped_hint = remapped + kProcessLogicNotes;
    assert(remapped_hint >= 0x732f36e000ull && remapped_hint < 0x7330414000ull);

    const uintptr_t contiguous = ProcMaps::FindLibraryBaseFromMaps(kContiguousLoad, kSoname);
    assert(contiguous == 0x7329ae6000ull);
    assert(contiguous + kProcessLogicNotes >= 0x732a6d4000ull);
    assert(contiguous + kProcessLogicNotes < 0x732a6d5000ull);

    assert(ProcMaps::FindLibraryBaseFromMaps(kDeletedSuffix, kSoname) == 0x72aa000000ull);
    assert(ProcMaps::FindLibraryBaseFromMaps(kAndroid16RelroBelowRx, "libfmod.so") == 0);
    assert(ProcMaps::FindLibraryBaseFromMaps("", kSoname) == 0);
    assert(ProcMaps::FindLibraryBaseFromMaps("73298ec000-7329a00000 r--p 0193c000 fe:28 1  /x/libcocos2dcpp.so\n",
                                             kSoname) == 0);
    return 0;
}
