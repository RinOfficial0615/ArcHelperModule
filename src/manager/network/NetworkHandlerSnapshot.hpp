#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace arc_helper::network {

struct HandlerArgs;
using HandlerFn = bool (*)(HandlerArgs &args);

// An immutable, sorted view of the handlers visible to hook threads.
// Registration creates a new snapshot; dispatch never observes a vector that
// another thread can resize or reorder.
class HandlerSnapshot final {
public:
    struct Entry {
        std::string name;
        int priority = 0;
        uint32_t register_order = 0;
        HandlerFn fn = nullptr;
    };

    static std::shared_ptr<const HandlerSnapshot> Empty();

    HandlerSnapshot() = default;

    std::span<const Entry> Entries() const { return entries_; }
    bool Contains(std::string_view name, HandlerFn fn) const;
    std::shared_ptr<const HandlerSnapshot> With(Entry entry) const;

private:
    std::vector<Entry> entries_{};
};

} // namespace arc_helper::network
