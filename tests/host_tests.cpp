#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "manager/network/NetworkHandler.hpp"
#include "utils/Sha256.hpp"
#include "utils/ZipArchive.hpp"

namespace {

bool LowHandler(arc_helper::network::HandlerArgs &) { return false; }
bool HighHandler(arc_helper::network::HandlerArgs &) { return false; }

} // namespace

int main(int argc, char **argv) {
    using namespace arc_helper;
    {
        const auto parsed = nlohmann::json::parse(
            R"({"ok":true,"n":12.5,"s":"\u4f60\u597d","a":[null,false]})", nullptr, false);
        assert(!parsed.is_discarded());
        assert(parsed.at("ok").get<bool>());
        assert(parsed.at("s").get<std::string>() == "你好");
        assert(nlohmann::json::parse(R"({"x":tru})", nullptr, false).is_discarded());
    }
    assert(crypto::Sha256Hex("abc", 3) ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(std::string_view(network::HttpMethodStr(0)) == "GET");
    assert(std::string_view(network::HttpMethodStr(3)) == "DELETE");
    assert(std::string_view(network::HttpMethodStr(99)) == "UNK");
    assert(network::HttpMethodBit(0) == cfg::network_block::kMethodGet);
    assert(network::HttpMethodBit(3) == cfg::network_block::kMethodDelete);
    assert(network::HttpMethodBit(99) == 0);

    {
        const auto empty = network::HandlerSnapshot::Empty();
        const auto low = empty->With({"low", 10, 0, LowHandler});
        const auto ordered = low->With({"high", 20, 1, HighHandler});
        assert(empty->Entries().empty());
        assert(low->Entries().size() == 1);
        assert(ordered->Entries().size() == 2);
        assert(ordered->Entries()[0].name == "high");
        assert(ordered->Entries()[1].name == "low");
        assert(ordered->Contains("high", nullptr) == true);

        network::BufferView view{};
        view.data = reinterpret_cast<const uint8_t *>("payload");
        view.full_len = 7;
        view.show_len = 7;
        view.status = network::BufferViewStatus::Ok;
        const auto limited = view.Limit(3);
        assert(limited.show_len == 3);
        assert(limited.full_len == 7);
        assert(limited.Truncated());
    }

    for (int i = 1; i < argc; ++i) {
        bool expect_fail = false;
        std::string path = argv[i];
        if (path == "--expect-fail" && i + 1 < argc) {
            expect_fail = true;
            path = argv[++i];
        }
        zip::Archive archive;
        std::string error;
        const bool opened = archive.Open(path, error);
        if (expect_fail) {
            if (opened) {
                std::cerr << "unsafe archive unexpectedly opened: " << path << '\n';
                return 2;
            }
            continue;
        }
        if (!opened) {
            std::cerr << "archive open failed: " << path << ": " << error << '\n';
            return 3;
        }
        size_t files = 0;
        for (const auto &entry : archive.Entries()) {
            if (entry.directory) continue;
            ++files;
            if (entry.uncompressed_size <= 2 * 1024 * 1024) {
                std::vector<uint8_t> data;
                if (!archive.Extract(entry, data, error)) {
                    std::cerr << "extract failed: " << entry.name << ": " << error << '\n';
                    return 4;
                }
            }
        }
        if (!files) return 5;
    }
    std::cout << "host tests passed\n";
    return 0;
}
