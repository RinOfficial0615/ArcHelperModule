#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace arc_helper::crypto {

std::string Sha256Hex(const void *data, size_t size);
std::string Sha256FileHex(const std::string &path, std::string *error = nullptr);

} // namespace arc_helper::crypto
