#include "utils/MiniJson.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdlib>

namespace arc_helper::json {

std::optional<double> Value::AsNumber() const {
    if (const auto *v = std::get_if<double>(&data)) return *v;
    return std::nullopt;
}

std::optional<bool> Value::AsBool() const {
    if (const auto *v = std::get_if<bool>(&data)) return *v;
    return std::nullopt;
}

const Value *Value::Find(std::string_view key) const {
    const auto *object = AsObject();
    if (!object) return nullptr;
    const auto it = object->find(key);
    return it == object->end() ? nullptr : &it->second;
}

namespace {

class Parser {
public:
    Parser(std::string_view text, size_t max_depth) : text_(text), max_depth_(max_depth) {}

    ParseResult Run() {
        ParseResult result;
        SkipSpace();
        if (!ParseValue(result.value, 0)) {
            result.error = error_.empty() ? "invalid JSON" : error_;
            result.error_offset = pos_;
            return result;
        }
        SkipSpace();
        if (pos_ != text_.size()) {
            result.error = "trailing characters";
            result.error_offset = pos_;
        }
        return result;
    }

private:
    void SkipSpace() {
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') break;
            ++pos_;
        }
    }

    bool Fail(const char *message) {
        if (error_.empty()) error_ = message;
        return false;
    }

    bool ParseValue(Value &out, size_t depth) {
        if (depth > max_depth_) return Fail("maximum nesting exceeded");
        SkipSpace();
        if (pos_ >= text_.size()) return Fail("unexpected end");
        switch (text_[pos_]) {
        case '{': return ParseObject(out, depth + 1);
        case '[': return ParseArray(out, depth + 1);
        case '"': {
            std::string value;
            if (!ParseString(value)) return false;
            out.data = std::move(value);
            return true;
        }
        case 't': return ParseLiteral("true", true, out);
        case 'f': return ParseLiteral("false", false, out);
        case 'n': return ParseNull(out);
        default: return ParseNumber(out);
        }
    }

    bool ParseLiteral(std::string_view literal, bool value, Value &out) {
        if (text_.substr(pos_, literal.size()) != literal) return Fail("invalid literal");
        pos_ += literal.size();
        out.data = value;
        return true;
    }

    bool ParseNull(Value &out) {
        constexpr std::string_view literal = "null";
        if (text_.substr(pos_, literal.size()) != literal) return Fail("invalid null");
        pos_ += literal.size();
        out.data = nullptr;
        return true;
    }

    bool ParseObject(Value &out, size_t depth) {
        ++pos_;
        Value::Object object;
        SkipSpace();
        if (pos_ < text_.size() && text_[pos_] == '}') {
            ++pos_;
            out.data = std::move(object);
            return true;
        }
        for (;;) {
            SkipSpace();
            std::string key;
            if (!ParseString(key)) return false;
            SkipSpace();
            if (pos_ >= text_.size() || text_[pos_++] != ':') return Fail("expected colon");
            Value value;
            if (!ParseValue(value, depth)) return false;
            if (!object.emplace(std::move(key), std::move(value)).second) return Fail("duplicate object key");
            SkipSpace();
            if (pos_ >= text_.size()) return Fail("unterminated object");
            const char c = text_[pos_++];
            if (c == '}') break;
            if (c != ',') return Fail("expected object comma");
        }
        out.data = std::move(object);
        return true;
    }

    bool ParseArray(Value &out, size_t depth) {
        ++pos_;
        Value::Array array;
        SkipSpace();
        if (pos_ < text_.size() && text_[pos_] == ']') {
            ++pos_;
            out.data = std::move(array);
            return true;
        }
        for (;;) {
            Value value;
            if (!ParseValue(value, depth)) return false;
            array.push_back(std::move(value));
            SkipSpace();
            if (pos_ >= text_.size()) return Fail("unterminated array");
            const char c = text_[pos_++];
            if (c == ']') break;
            if (c != ',') return Fail("expected array comma");
        }
        out.data = std::move(array);
        return true;
    }

    static void AppendUtf8(std::string &out, uint32_t cp) {
        if (cp <= 0x7F) out.push_back(static_cast<char>(cp));
        else if (cp <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }

    static int Hex(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    bool ReadHex4(uint32_t &value) {
        if (pos_ + 4 > text_.size()) return Fail("short unicode escape");
        value = 0;
        for (int i = 0; i < 4; ++i) {
            const int h = Hex(text_[pos_++]);
            if (h < 0) return Fail("invalid unicode escape");
            value = (value << 4) | static_cast<uint32_t>(h);
        }
        return true;
    }

    bool ParseString(std::string &out) {
        if (pos_ >= text_.size() || text_[pos_++] != '"') return Fail("expected string");
        while (pos_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[pos_++]);
            if (c == '"') return true;
            if (c < 0x20) return Fail("control character in string");
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            if (pos_ >= text_.size()) return Fail("short string escape");
            const char escaped = text_[pos_++];
            switch (escaped) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                uint32_t cp = 0;
                if (!ReadHex4(cp)) return false;
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if (pos_ + 2 > text_.size() || text_[pos_] != '\\' || text_[pos_ + 1] != 'u')
                        return Fail("missing low surrogate");
                    pos_ += 2;
                    uint32_t low = 0;
                    if (!ReadHex4(low) || low < 0xDC00 || low > 0xDFFF) return Fail("invalid low surrogate");
                    cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    return Fail("unexpected low surrogate");
                }
                AppendUtf8(out, cp);
                break;
            }
            default: return Fail("invalid string escape");
            }
        }
        return Fail("unterminated string");
    }

    bool ParseNumber(Value &out) {
        const size_t begin = pos_;
        if (text_[pos_] == '-') ++pos_;
        if (pos_ >= text_.size()) return Fail("invalid number");
        if (text_[pos_] == '0') ++pos_;
        else {
            if (text_[pos_] < '1' || text_[pos_] > '9') return Fail("invalid number");
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
        }
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            const size_t fraction = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            if (fraction == pos_) return Fail("invalid number fraction");
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) ++pos_;
            const size_t exponent = pos_;
            while (pos_ < text_.size() && text_[pos_] >= '0' && text_[pos_] <= '9') ++pos_;
            if (exponent == pos_) return Fail("invalid number exponent");
        }
        const std::string token(text_.substr(begin, pos_ - begin));
        char *end = nullptr;
        const double value = std::strtod(token.c_str(), &end);
        if (!end || *end != '\0' || !std::isfinite(value)) return Fail("invalid numeric value");
        out.data = value;
        return true;
    }

    std::string_view text_;
    size_t max_depth_ = 64;
    size_t pos_ = 0;
    std::string error_{};
};

} // namespace

ParseResult Parse(std::string_view text, size_t max_depth) {
    return Parser(text, max_depth).Run();
}

std::string Escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 8);
    constexpr char hex[] = "0123456789ABCDEF";
    for (const unsigned char c : text) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (c < 0x20) {
                out += "\\u00";
                out.push_back(hex[c >> 4]);
                out.push_back(hex[c & 0x0F]);
            } else {
                out.push_back(static_cast<char>(c));
            }
        }
    }
    return out;
}

} // namespace arc_helper::json
