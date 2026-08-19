#pragma once

#include <string>
#include <string_view>

namespace arc_helper::aff {

// Faithful 6.16.2c TokenLexer + parseNote check (IDA sub_93A310 / sub_8DE248).
// Does not execute gameplay; it only accepts the official token grammar.
struct OfficialCheck {
    bool ok = true;
    int line = 0;
    std::string error;
};

OfficialCheck CheckOfficial(std::string_view text);

} // namespace arc_helper::aff
