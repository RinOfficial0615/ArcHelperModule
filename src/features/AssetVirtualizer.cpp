#include "features/AssetVirtualizer.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <android/asset_manager.h>

#include "config/ModuleConfig.h"
#include "features/CustomChartManager.hpp"
#include "features/CustomSession.hpp"
#include "manager/GameManager.hpp"
#include "third_party/lsplt/include/lsplt.hpp"
#include "utils/Log.h"

namespace arc_helper {
namespace {

struct VirtualAsset {
    std::shared_ptr<std::vector<uint8_t>> data;
    size_t position = 0;
    std::string logical_path;
};

std::mutex g_assets_mutex;
std::unordered_map<AAsset *, VirtualAsset> g_assets;
uintptr_t g_lib_base = 0;
cfg::CustomChartsOffsets g_offsets{};

using OpenFn = AAsset *(*)(AAssetManager *, const char *, int);
using ReadFn = int (*)(AAsset *, void *, size_t);
using LengthFn = off_t (*)(AAsset *);
using CloseFn = void (*)(AAsset *);

OpenFn g_open_original = nullptr;
ReadFn g_read_original = nullptr;
LengthFn g_length_original = nullptr;
CloseFn g_close_original = nullptr;

bool EndsWith(std::string_view value, std::string_view suffix) {
    return value.size() >= suffix.size() && value.substr(value.size() - suffix.size()) == suffix;
}

std::shared_ptr<std::vector<uint8_t>> ReadFile(const std::string &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    const auto size = file.tellg();
    if (size < 0 || static_cast<uint64_t>(size) > 128ull * 1024 * 1024) return {};
    auto data = std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(size));
    file.seekg(0);
    if (!data->empty() && !file.read(reinterpret_cast<char *>(data->data()), data->size())) return {};
    return data;
}

std::shared_ptr<std::vector<uint8_t>> ReadOfficialAsset(AAsset *asset) {
    if (!asset || !g_length_original || !g_read_original) return {};
    const off_t length = g_length_original(asset);
    if (length <= 0 || static_cast<uint64_t>(length) > 64ull * 1024 * 1024) return {};
    auto data = std::make_shared<std::vector<uint8_t>>(static_cast<size_t>(length));
    size_t done = 0;
    while (done < data->size()) {
        const int count = g_read_original(asset, data->data() + done, data->size() - done);
        if (count <= 0) return {};
        done += static_cast<size_t>(count);
    }
    return data;
}

bool IsParserCaller(uintptr_t return_address) {
    if (!g_lib_base || !g_offsets.songlist_parser || return_address < g_lib_base) return false;
    const uintptr_t offset = return_address - g_lib_base;
    // Confirmed parser body range for 6.16.2c. Keep the window bounded so the
    // separate integrity/preload path continues to receive official bytes.
    return offset >= g_offsets.songlist_parser && offset < g_offsets.songlist_parser + 0x77E8;
}

AAsset *OpenHook(AAssetManager *manager, const char *filename, int mode) {
    if (!g_open_original) return nullptr;
    if (!filename) return g_open_original(manager, filename, mode);
    const std::string_view path(filename);
    auto &charts = CustomChartManager::Instance();

    if (path == "songs/songlist") {
        AAsset *asset = g_open_original(manager, filename, mode);
        if (!asset || !IsParserCaller(reinterpret_cast<uintptr_t>(__builtin_return_address(0)))) return asset;
        const auto official = ReadOfficialAsset(asset);
        if (!official) {
            ARC_LOGE("AssetVirtualizer: failed to read official songlist");
            return asset;
        }
        std::string error;
        const std::string merged = charts.MergeSonglist(
            std::string_view(reinterpret_cast<const char *>(official->data()), official->size()), error);
        if (merged.empty()) {
            ARC_LOGE("AssetVirtualizer: songlist merge failed: %s", error.c_str());
            g_close_original(asset);
            return g_open_original(manager, filename, mode);
        }
        auto bytes = std::make_shared<std::vector<uint8_t>>(merged.begin(), merged.end());
        {
            std::scoped_lock lock(g_assets_mutex);
            g_assets[asset] = {std::move(bytes), 0, std::string(path)};
        }
        ARC_LOGI("AssetVirtualizer: serving merged songlist (%zu -> %zu bytes)", official->size(), merged.size());
        return asset;
    }

    const std::string *source = charts.ResolveAsset(path);
    if (!source) {
        if (CustomSession::Instance().IsActive() && EndsWith(path, ".aff")) {
            CustomSession::Instance().Clear("official-chart");
        }
        if (CustomSession::Instance().IsActive() && path.starts_with("songs/") &&
            (EndsWith(path, "/base.jpg") || EndsWith(path, "/base_256.jpg"))) {
            CustomSession::Instance().Clear("official-song-asset");
        }
        return g_open_original(manager, filename, mode);
    }

    if (source->starts_with("@official:")) {
        return g_open_original(manager, source->c_str() + std::strlen("@official:"), mode);
    }

    const auto bytes = ReadFile(*source);
    if (!bytes) {
        ARC_LOGE("AssetVirtualizer: failed to read %s -> %s", filename, source->c_str());
        return nullptr;
    }
    AAsset *asset = g_open_original(manager, "songs/songlist", mode);
    if (!asset) return nullptr;
    {
        std::scoped_lock lock(g_assets_mutex);
        g_assets[asset] = {bytes, 0, std::string(path)};
    }
    std::string song_id;
    if (charts.IsCustomChartPath(path, &song_id)) CustomSession::Instance().Activate(song_id.c_str());
    ARC_LOGI("AssetVirtualizer: mapped %s (%zu bytes)", filename, bytes->size());
    return asset;
}

int ReadHook(AAsset *asset, void *buffer, size_t count) {
    {
        std::scoped_lock lock(g_assets_mutex);
        const auto it = g_assets.find(asset);
        if (it != g_assets.end()) {
            if (!buffer || !it->second.data) return -1;
            const size_t remaining = it->second.position < it->second.data->size()
                                         ? it->second.data->size() - it->second.position : 0;
            const size_t n = std::min(count, remaining);
            if (n) std::memcpy(buffer, it->second.data->data() + it->second.position, n);
            it->second.position += n;
            return static_cast<int>(n);
        }
    }
    return g_read_original ? g_read_original(asset, buffer, count) : -1;
}

off_t LengthHook(AAsset *asset) {
    {
        std::scoped_lock lock(g_assets_mutex);
        const auto it = g_assets.find(asset);
        if (it != g_assets.end() && it->second.data)
            return static_cast<off_t>(it->second.data->size());
    }
    return g_length_original ? g_length_original(asset) : 0;
}

void CloseHook(AAsset *asset) {
    {
        std::scoped_lock lock(g_assets_mutex);
        g_assets.erase(asset);
    }
    if (g_close_original) g_close_original(asset);
}

} // namespace

AssetVirtualizer &AssetVirtualizer::Instance() {
    static AssetVirtualizer virtualizer;
    return virtualizer;
}

bool AssetVirtualizer::Install(const cfg::GameProfile &profile) {
    if (installed_) return true;
    lib_base_ = GameManager::Instance().GetOrFindGameLibBase();
    if (!lib_base_) return false;
    g_lib_base = lib_base_;
    offsets_ = profile.custom_charts;
    g_offsets = offsets_;

    dev_t dev = 0;
    ino_t inode = 0;
    for (const auto &map : lsplt::MapInfo::Scan()) {
        if (map.path.ends_with(cfg::module::kLibName)) {
            dev = map.dev;
            inode = map.inode;
            break;
        }
    }
    if (!inode) {
        ARC_LOGE("AssetVirtualizer: target inode not found");
        return false;
    }

    const bool registered =
        lsplt::RegisterHook(dev, inode, "AAssetManager_open", reinterpret_cast<void *>(OpenHook),
                            reinterpret_cast<void **>(&g_open_original)) &&
        lsplt::RegisterHook(dev, inode, "AAsset_read", reinterpret_cast<void *>(ReadHook),
                            reinterpret_cast<void **>(&g_read_original)) &&
        lsplt::RegisterHook(dev, inode, "AAsset_getLength", reinterpret_cast<void *>(LengthHook),
                            reinterpret_cast<void **>(&g_length_original)) &&
        lsplt::RegisterHook(dev, inode, "AAsset_close", reinterpret_cast<void *>(CloseHook),
                            reinterpret_cast<void **>(&g_close_original));
    installed_ = registered && lsplt::CommitHook() && g_open_original && g_read_original &&
                 g_length_original && g_close_original;
    ARC_LOGI("AssetVirtualizer: hook install %s", installed_ ? "OK" : "FAILED");
    return installed_;
}

} // namespace arc_helper
