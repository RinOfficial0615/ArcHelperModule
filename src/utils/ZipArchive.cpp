#include "utils/ZipArchive.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <limits>
#include <set>

#include <zlib.h>

namespace arc_helper::zip {
namespace {

constexpr uint32_t kLocalSignature = 0x04034B50;
constexpr uint32_t kCentralSignature = 0x02014B50;
constexpr uint32_t kEocdSignature = 0x06054B50;
constexpr size_t kMaxArchiveBytes = 512ull * 1024 * 1024;
constexpr size_t kMaxEntryBytes = 128ull * 1024 * 1024;
constexpr size_t kMaxTotalOutputBytes = 768ull * 1024 * 1024;
constexpr size_t kMaxEntries = 4096;

uint16_t U16(const uint8_t *p) { return static_cast<uint16_t>(p[0] | (p[1] << 8)); }
uint32_t U32(const uint8_t *p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

} // namespace

bool Archive::NormalizePath(std::string_view input, std::string &output) {
    output.clear();
    if (input.empty() || input.size() > 1024 || input.front() == '/' || input.front() == '\\') return false;
    std::string component;
    auto flush = [&]() -> bool {
        if (component.empty() || component == "." || component == "..") return false;
        if (component.find(':') != std::string::npos || component.find('\0') != std::string::npos) return false;
        if (!output.empty()) output.push_back('/');
        output += component;
        component.clear();
        return true;
    };
    for (char c : input) {
        if (c == '/' || c == '\\') {
            if (!flush()) return false;
        } else {
            component.push_back(c);
        }
    }
    if (!component.empty() && !flush()) return false;
    return !output.empty();
}

bool Archive::Open(const std::string &path, std::string &error) {
    bytes_.clear(); entries_.clear(); by_name_.clear(); error.clear();
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { error = "open failed"; return false; }
    const auto end = file.tellg();
    if (end <= 0 || static_cast<uint64_t>(end) > kMaxArchiveBytes) { error = "archive size limit"; return false; }
    bytes_.resize(static_cast<size_t>(end));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char *>(bytes_.data()), bytes_.size())) { error = "read failed"; return false; }
    if (bytes_.size() < 22) { error = "archive too short"; return false; }

    const size_t search_begin = bytes_.size() > 65557 ? bytes_.size() - 65557 : 0;
    size_t eocd = std::numeric_limits<size_t>::max();
    for (size_t p = bytes_.size() - 22;; --p) {
        if (U32(bytes_.data() + p) == kEocdSignature) { eocd = p; break; }
        if (p == search_begin) break;
    }
    if (eocd == std::numeric_limits<size_t>::max() || eocd + 22 > bytes_.size()) { error = "EOCD not found"; return false; }
    const uint16_t disk = U16(bytes_.data() + eocd + 4);
    const uint16_t central_disk = U16(bytes_.data() + eocd + 6);
    const uint16_t disk_count = U16(bytes_.data() + eocd + 8);
    const uint16_t count = U16(bytes_.data() + eocd + 10);
    const uint32_t central_size = U32(bytes_.data() + eocd + 12);
    const uint32_t central_offset = U32(bytes_.data() + eocd + 16);
    const uint16_t comment_len = U16(bytes_.data() + eocd + 20);
    if (eocd + 22ull + comment_len != bytes_.size()) { error = "invalid EOCD comment bounds"; return false; }
    if (disk || central_disk || disk_count != count || count == 0xFFFF ||
        central_size == 0xFFFFFFFF || central_offset == 0xFFFFFFFF) {
        error = "multi-disk/ZIP64 unsupported"; return false;
    }
    if (count > kMaxEntries || static_cast<uint64_t>(central_offset) + central_size > bytes_.size()) {
        error = "central directory bounds"; return false;
    }

    size_t p = central_offset;
    size_t total_output = 0;
    for (uint16_t i = 0; i < count; ++i) {
        const size_t central_end = static_cast<size_t>(central_offset) + central_size;
        if (p + 46 > central_end || U32(bytes_.data() + p) != kCentralSignature) {
            error = "invalid central entry"; return false;
        }
        Entry entry;
        entry.flags = U16(bytes_.data() + p + 8);
        entry.method = U16(bytes_.data() + p + 10);
        entry.crc32 = U32(bytes_.data() + p + 16);
        entry.compressed_size = U32(bytes_.data() + p + 20);
        entry.uncompressed_size = U32(bytes_.data() + p + 24);
        const uint16_t name_len = U16(bytes_.data() + p + 28);
        const uint16_t extra_len = U16(bytes_.data() + p + 30);
        const uint16_t comment_len = U16(bytes_.data() + p + 32);
        const uint32_t external_attr = U32(bytes_.data() + p + 38);
        entry.local_header_offset = U32(bytes_.data() + p + 42);
        const size_t next = p + 46ull + name_len + extra_len + comment_len;
        if (!name_len || next > central_end) { error = "central name bounds"; return false; }
        const std::string_view raw(reinterpret_cast<const char *>(bytes_.data() + p + 46), name_len);
        const bool raw_dir = raw.back() == '/' || raw.back() == '\\';
        std::string trimmed(raw);
        while (!trimmed.empty() && (trimmed.back() == '/' || trimmed.back() == '\\')) trimmed.pop_back();
        if (!NormalizePath(trimmed, entry.name)) { error = "unsafe path"; return false; }
        entry.directory = raw_dir;
        const uint32_t unix_mode = (external_attr >> 16) & 0xF000;
        if (unix_mode == 0xA000) { error = "symlink entry rejected"; return false; }
        if ((entry.flags & 1u) != 0 || (entry.method != 0 && entry.method != 8)) {
            error = "encrypted/unsupported compression"; return false;
        }
        if (!entry.directory) {
            if (entry.uncompressed_size > kMaxEntryBytes) { error = "entry size limit"; return false; }
            if (entry.uncompressed_size > 1024 * 1024 && entry.compressed_size > 0 &&
                entry.uncompressed_size / entry.compressed_size > 200) {
                error = "compression ratio limit"; return false;
            }
            total_output += entry.uncompressed_size;
            if (total_output > kMaxTotalOutputBytes) { error = "total output limit"; return false; }
        }
        if (!by_name_.emplace(entry.name, entries_.size()).second) { error = "duplicate normalized path"; return false; }
        entries_.push_back(std::move(entry));
        p = next;
    }
    if (p != static_cast<size_t>(central_offset) + central_size || p != eocd) {
        error = "central directory size mismatch"; return false;
    }
    return true;
}

const Entry *Archive::Find(std::string_view normalized_name) const {
    const auto it = by_name_.find(std::string(normalized_name));
    return it == by_name_.end() ? nullptr : &entries_[it->second];
}

bool Archive::Extract(const Entry &entry, std::vector<uint8_t> &out, std::string &error) const {
    out.clear(); error.clear();
    if (entry.directory || entry.local_header_offset + 30ull > bytes_.size()) { error = "local header bounds"; return false; }
    const uint8_t *header = bytes_.data() + entry.local_header_offset;
    if (U32(header) != kLocalSignature) { error = "invalid local header"; return false; }
    const uint16_t local_flags = U16(header + 6);
    const uint16_t local_method = U16(header + 8);
    const uint16_t name_len = U16(header + 26);
    const uint16_t extra_len = U16(header + 28);
    const size_t data_offset = entry.local_header_offset + 30ull + name_len + extra_len;
    if (data_offset + entry.compressed_size > bytes_.size()) { error = "compressed data bounds"; return false; }
    if (local_flags != entry.flags || local_method != entry.method || name_len == 0) {
        error = "local/central header mismatch"; return false;
    }
    std::string local_name;
    const std::string_view raw_local_name(reinterpret_cast<const char *>(header + 30), name_len);
    std::string trimmed_local_name(raw_local_name);
    while (!trimmed_local_name.empty() &&
           (trimmed_local_name.back() == '/' || trimmed_local_name.back() == '\\')) {
        trimmed_local_name.pop_back();
    }
    if (!NormalizePath(trimmed_local_name, local_name) || local_name != entry.name) {
        error = "local/central name mismatch"; return false;
    }
    out.resize(entry.uncompressed_size);
    const uint8_t *source = bytes_.data() + data_offset;
    if (entry.method == 0) {
        if (entry.compressed_size != entry.uncompressed_size) { error = "stored size mismatch"; return false; }
        std::copy(source, source + entry.compressed_size, out.begin());
    } else {
        z_stream stream{};
        stream.next_in = const_cast<Bytef *>(source);
        stream.avail_in = entry.compressed_size;
        stream.next_out = out.data();
        stream.avail_out = entry.uncompressed_size;
        if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) { error = "inflate init"; return false; }
        const int rc = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);
        if (rc != Z_STREAM_END || stream.total_out != entry.uncompressed_size) { error = "inflate failed"; return false; }
    }
    const uLong crc = ::crc32(0, out.data(), static_cast<uInt>(out.size()));
    if (static_cast<uint32_t>(crc) != entry.crc32) { error = "CRC mismatch"; return false; }
    return true;
}

bool Archive::ExtractToFile(const Entry &entry, const std::string &path, std::string &error) const {
    std::vector<uint8_t> data;
    if (!Extract(entry, data, error)) return false;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    if (ec) { error = "create directories failed"; return false; }
    const std::string tmp = path + ".tmp";
    std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
    if (!file || !file.write(reinterpret_cast<const char *>(data.data()), data.size())) {
        error = "write failed"; return false;
    }
    file.close();
    std::filesystem::rename(tmp, path, ec);
    if (ec) { std::filesystem::remove(tmp); error = "atomic rename failed"; return false; }
    return true;
}

} // namespace arc_helper::zip
