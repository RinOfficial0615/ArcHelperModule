#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace arc_helper::cfg::custom_charts {

// Bounded parser values. User-facing defaults are owned by CustomCharts.
inline constexpr double kMinimumBpm = 1.0;
inline constexpr double kMaximumBpm = 10000.0;
inline constexpr int kMinimumSide = 0;
inline constexpr int kMaximumSide = 2;
inline constexpr const char *kLightBackground = "base_light";
inline constexpr const char *kConflictBackground = "base_conflict";
inline constexpr double kDefaultChartConstant = -1.0;
inline constexpr int64_t kMaximumPreviewEndMs = INT32_MAX;
inline constexpr int kMinimumRating = 0;
inline constexpr int kMaximumRating = 20;
inline constexpr int kMinimumChartConstant = 0;

inline constexpr size_t kDifficultyCount = 5;
inline constexpr size_t kPastDifficulty = 0;
inline constexpr size_t kPresentDifficulty = 1;
inline constexpr size_t kFutureDifficulty = 2;
inline constexpr size_t kBeyondDifficulty = 3;
inline constexpr size_t kEternalDifficulty = 4;
inline constexpr size_t kSongIdHashChars = 8;
inline constexpr size_t kMaxSongIdLength = 21;
inline constexpr size_t kMaxSanitizedIdLength = 48;
inline constexpr size_t kMaxTextEntryBytes = 8 * 1024 * 1024;
inline constexpr size_t kMaxMetadataStringBytes = 256;
inline constexpr uint64_t kMaxVirtualAssetBytes = 128ull * 1024 * 1024;
inline constexpr uint64_t kMaxOfficialAssetBytes = 64ull * 1024 * 1024;

// Runtime song object/list layout confirmed for the supported custom-chart build.
inline constexpr uintptr_t kDifficultyPointersOffset = 0x228;
inline constexpr uintptr_t kDifficultyPresenceOffset = 0x250;
inline constexpr size_t kDifficultyObjectReadableBytes = 0x50;
inline constexpr uintptr_t kSongRegistryOwnerRegistryOffset = 32;
inline constexpr size_t kMaxRuntimeDifficultyPairs = 4096;

// Asset namespaces and importer-generated aliases.
inline constexpr std::string_view kSongsPrefix = "songs/";
inline constexpr std::string_view kSonglistAssetPath = "songs/songlist";
inline constexpr std::string_view kOfficialAssetPrefix = "@official:";
inline constexpr std::string_view kCustomSongIdPrefix = "ah_";
inline constexpr std::string_view kCustomBackgroundPrefix = "ahbg_";
inline constexpr std::string_view kBackgroundAssetPrefix = "img/bg/1080/";
inline constexpr std::array<std::string_view, 3> kAssetPathPrefixes = {
    "file:///android_asset/", "Resources/", "assets/",
};
inline constexpr std::array<const char *, 2> kAudioFileNames = {"base.ogg", "base.wav"};
inline constexpr std::array<const char *, 3> kJacketFileNames = {
    "base.jpg", "base.png", "base.jpeg",
};
inline constexpr const char *kAudioAssetName = "base.ogg";
inline constexpr const char *kJacketAssetName = "base.jpg";
inline constexpr const char *kJacket256AssetName = "base_256.jpg";
inline constexpr const char *kExtractedAudioStem = "/base";
inline constexpr const char *kExtractedJacketStem = "/jacket";
inline constexpr const char *kExtractedBackgroundStem = "/bg";
inline constexpr const char *kDefaultJacketAsset = "@official:img/default_jacket.jpg";
inline constexpr const char *kDefaultJacket256Asset = "@official:img/default_jacket_256.jpg";
inline constexpr const char *kSongSet = "base";
inline constexpr const char *kSongPurchase = "";
inline constexpr const char *kSongVersion = "4.0.0";
inline constexpr int kSongDate = 0;

// Runtime patch and hook signatures.
inline constexpr uint32_t kNopInstruction = 0xD503201F;
inline constexpr uint32_t kExpectedSonglistLoaderCall = 0x94140479;
inline constexpr uint32_t kExpectedDigestSizeGuard = 0x540013A1;
inline constexpr uint32_t kExpectedDigestCompareGuard = 0x350012C0;

inline constexpr std::array<uint8_t, 16> kSigCxaThrow = {
    0x3F, 0x23, 0x03, 0xD5, 0xFD, 0x7B, 0xBC, 0xA9,
    0xF7, 0x0B, 0x00, 0xF9, 0xF6, 0x57, 0x02, 0xA9,
};

inline constexpr std::array<uint8_t, 16> kSigSonglistDifficultyFilter = {
    0xFF, 0x83, 0x03, 0xD1, 0xFD, 0x7B, 0x08, 0xA9,
    0xFC, 0x6F, 0x09, 0xA9, 0xFA, 0x67, 0x0A, 0xA9,
};

inline constexpr std::array<uint8_t, 16> kSigFindSongById = {
    0xFD, 0x7B, 0xBE, 0xA9, 0xF3, 0x0B, 0x00, 0xF9,
    0xFD, 0x03, 0x00, 0x91, 0xF3, 0x03, 0x00, 0xAA,
};

inline constexpr std::array<uint8_t, 16> kSigFmodLoadBgm = {
    0xFF, 0x03, 0x04, 0xD1, 0xFD, 0x7B, 0x0A, 0xA9,
    0xFC, 0x6F, 0x0B, 0xA9, 0xFA, 0x67, 0x0C, 0xA9,
};

inline constexpr const char *kCxaThrowSymbol = "__cxa_throw";
inline constexpr const char *kFmodProviderLibrary = "libfmodProvider.so";
inline constexpr const char *kFmodLoadBgmSymbol =
    "_ZN24AudioProviderFMODAndroid7loadBGMEPKci";

} // namespace arc_helper::cfg::custom_charts
