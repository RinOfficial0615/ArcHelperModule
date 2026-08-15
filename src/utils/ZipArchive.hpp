#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace arc_helper::zip {

struct Entry {
    std::string name;
    uint16_t flags = 0;
    uint16_t method = 0;
    uint32_t crc32 = 0;
    uint32_t compressed_size = 0;
    uint32_t uncompressed_size = 0;
    uint32_t local_header_offset = 0;
    bool directory = false;
};

class Archive {
public:
    bool Open(const std::string &path, std::string &error);
    const std::vector<Entry> &Entries() const { return entries_; }
    const Entry *Find(std::string_view normalized_name) const;
    bool Extract(const Entry &entry, std::vector<uint8_t> &out, std::string &error) const;
    bool ExtractToFile(const Entry &entry, const std::string &path, std::string &error) const;

    static bool NormalizePath(std::string_view input, std::string &output);

private:
    std::vector<uint8_t> bytes_{};
    std::vector<Entry> entries_{};
    std::unordered_map<std::string, size_t> by_name_{};
};

} // namespace arc_helper::zip
