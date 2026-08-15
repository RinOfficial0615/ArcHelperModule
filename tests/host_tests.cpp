#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "utils/MiniJson.hpp"
#include "utils/Sha256.hpp"
#include "utils/ZipArchive.hpp"

int main(int argc, char **argv) {
    using namespace arc_helper;
    {
        const auto parsed = json::Parse(R"({"ok":true,"n":12.5,"s":"\u4f60\u597d","a":[null,false]})");
        assert(parsed);
        assert(parsed.value.Find("ok")->AsBool().value());
        assert(parsed.value.Find("s") && *parsed.value.Find("s")->AsString() == "你好");
        assert(!json::Parse(R"({"x":1,"x":2})"));
        assert(!json::Parse(R"({"x":tru})"));
    }
    assert(crypto::Sha256Hex("abc", 3) ==
           "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

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
