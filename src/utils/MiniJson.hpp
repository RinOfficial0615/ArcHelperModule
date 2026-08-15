#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace arc_helper::json {

struct Value {
    using Array = std::vector<Value>;
    using Object = std::map<std::string, Value, std::less<>>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    Storage data{nullptr};

    bool IsNull() const { return std::holds_alternative<std::nullptr_t>(data); }
    bool IsBool() const { return std::holds_alternative<bool>(data); }
    bool IsNumber() const { return std::holds_alternative<double>(data); }
    bool IsString() const { return std::holds_alternative<std::string>(data); }
    bool IsArray() const { return std::holds_alternative<Array>(data); }
    bool IsObject() const { return std::holds_alternative<Object>(data); }

    const Array *AsArray() const { return std::get_if<Array>(&data); }
    const Object *AsObject() const { return std::get_if<Object>(&data); }
    const std::string *AsString() const { return std::get_if<std::string>(&data); }
    std::optional<double> AsNumber() const;
    std::optional<bool> AsBool() const;
    const Value *Find(std::string_view key) const;
};

struct ParseResult {
    Value value{};
    std::string error{};
    size_t error_offset = 0;
    explicit operator bool() const { return error.empty(); }
};

ParseResult Parse(std::string_view text, size_t max_depth = 64);
std::string Escape(std::string_view text);

} // namespace arc_helper::json
