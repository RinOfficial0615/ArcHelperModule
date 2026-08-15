#include "features/CustomChartManager.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <sstream>

#include <nlohmann/json.hpp>

#include "config/RuntimeConfig.hpp"
#include "features/AssetVirtualizer.hpp"
#include "utils/Log.h"
#include "utils/Sha256.hpp"
#include "utils/ZipArchive.hpp"

namespace arc_helper {
namespace {

using zip::Archive;
using zip::Entry;
using Json = nlohmann::json;

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string Trim(std::string_view value) {
    size_t b = 0, e = value.size();
    while (b < e && std::isspace(static_cast<unsigned char>(value[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(value[e - 1]))) --e;
    std::string out(value.substr(b, e - b));
    if (out.size() >= 2 && ((out.front() == '\'' && out.back() == '\'') ||
                            (out.front() == '"' && out.back() == '"'))) {
        out = out.substr(1, out.size() - 2);
    }
    return out;
}

std::string BaseName(std::string_view path) {
    const size_t slash = path.find_last_of("/\\");
    return std::string(path.substr(slash == std::string_view::npos ? 0 : slash + 1));
}

std::string Stem(std::string_view path) {
    std::string base = BaseName(path);
    const size_t dot = base.find_last_of('.');
    if (dot != std::string::npos) base.resize(dot);
    return base;
}

std::string Extension(std::string_view path) {
    const std::string base = BaseName(path);
    const size_t dot = base.find_last_of('.');
    return dot == std::string::npos ? std::string{} : Lower(base.substr(dot));
}

std::string SanitizeId(std::string_view input) {
    std::string out;
    out.reserve(std::min<size_t>(input.size(), 48));
    for (const unsigned char c : input) {
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
        else if ((c == '_' || c == '-') && !out.empty() && out.back() != '_') out.push_back('_');
        if (out.size() == 48) break;
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out.empty() ? "chart" : out;
}

std::string MakeSongId(std::string_view source, std::string_view hash, size_t ordinal = 0) {
    std::string id = "ah_" + SanitizeId(source) + "_" + std::string(hash.substr(0, 8));
    if (ordinal) id += "_" + std::to_string(ordinal);
    return id;
}

bool ReadEntryText(const Archive &archive, const Entry *entry, std::string &out, std::string &error) {
    if (!entry) { error = "entry missing"; return false; }
    std::vector<uint8_t> data;
    if (!archive.Extract(*entry, data, error)) return false;
    if (data.size() > 8 * 1024 * 1024) { error = "text entry size limit"; return false; }
    out.assign(reinterpret_cast<const char *>(data.data()), data.size());
    return true;
}

const Entry *FindCaseInsensitive(const Archive &archive, std::string_view path) {
    const std::string wanted = Lower(std::string(path));
    for (const auto &entry : archive.Entries()) {
        if (!entry.directory && Lower(entry.name) == wanted) return &entry;
    }
    return nullptr;
}

const Entry *FindOneByExtension(const Archive &archive, const std::set<std::string> &extensions,
                                std::string_view prefix = {}) {
    const Entry *found = nullptr;
    for (const auto &entry : archive.Entries()) {
        if (entry.directory || !extensions.contains(Extension(entry.name))) continue;
        if (!prefix.empty() && !entry.name.starts_with(prefix)) continue;
        if (found) return nullptr;
        found = &entry;
    }
    return found;
}

double FirstAffBpm(std::string_view text) {
    const size_t timing = text.find("timing(");
    if (timing == std::string_view::npos) return 120.0;
    const size_t comma1 = text.find(',', timing + 7);
    if (comma1 == std::string_view::npos) return 120.0;
    const size_t comma2 = text.find(',', comma1 + 1);
    if (comma2 == std::string_view::npos) return 120.0;
    const std::string token = Trim(text.substr(comma1 + 1, comma2 - comma1 - 1));
    char *end = nullptr;
    const double bpm = std::strtod(token.c_str(), &end);
    return end && *end == '\0' && std::isfinite(bpm) && bpm > 0 ? bpm : 120.0;
}

std::optional<int> SlotFromPathOrDifficulty(std::string_view path, std::string_view difficulty) {
    const std::string stem = Stem(path);
    if (stem.size() == 1 && stem[0] >= '0' && stem[0] <= '3') return stem[0] - '0';
    const std::string d = Lower(std::string(difficulty));
    if (d.find("past") != std::string::npos) return 0;
    if (d.find("present") != std::string::npos) return 1;
    if (d.find("future") != std::string::npos) return 2;
    if (d.find("beyond") != std::string::npos) return 3;
    return std::nullopt;
}

std::string FormatBpm(double bpm) {
    std::ostringstream out;
    if (std::floor(bpm) == bpm) out << static_cast<int64_t>(bpm);
    else out << bpm;
    return out.str();
}

bool AtomicWrite(const std::string &path, std::string_view data) {
    const std::string tmp = path + ".tmp";
    std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
    if (!file || !file.write(data.data(), static_cast<std::streamsize>(data.size()))) return false;
    file.close();
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) std::filesystem::remove(tmp);
    return !ec;
}

struct ArcIndexItem {
    std::string directory;
    std::string identifier;
    std::string settings_file;
    std::string type;
};

std::vector<ArcIndexItem> ParseArcIndex(std::string_view text) {
    std::vector<ArcIndexItem> items;
    ArcIndexItem current;
    bool active = false;
    std::istringstream lines{std::string(text)};
    std::string line;
    while (std::getline(lines, line)) {
        const std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        std::string content = trimmed;
        if (content.starts_with("- ")) {
            if (active && !current.directory.empty()) items.push_back(current);
            current = {};
            active = true;
            content = Trim(content.substr(2));
        }
        const size_t colon = content.find(':');
        if (!active || colon == std::string::npos) continue;
        const std::string key = Trim(content.substr(0, colon));
        const std::string value = Trim(content.substr(colon + 1));
        if (key == "directory") current.directory = value;
        else if (key == "identifier") current.identifier = value;
        else if (key == "settingsFile") current.settings_file = value;
        else if (key == "type") current.type = value;
    }
    if (active && !current.directory.empty()) items.push_back(current);
    return items;
}

struct ArcChartSettings {
    std::string chart_path, audio_path, jacket_path, background_path, title, composer, charter, illustrator, difficulty;
    std::string bpm_text;
    double base_bpm = 120.0;
    double chart_constant = -1.0;
    int64_t preview_start = 0, preview_end = 30000;
};

std::vector<ArcChartSettings> ParseArcProject(std::string_view text) {
    std::vector<ArcChartSettings> charts;
    ArcChartSettings current;
    bool in_charts = false, active = false;
    std::istringstream lines{std::string(text)};
    std::string line;
    while (std::getline(lines, line)) {
        const size_t indent = line.find_first_not_of(' ');
        const std::string trimmed = Trim(line);
        if (trimmed == "charts:") { in_charts = true; continue; }
        if (!in_charts || trimmed.empty() || trimmed[0] == '#') continue;
        if (indent == 0 && !trimmed.starts_with("- ")) break;
        std::string content = trimmed;
        if (indent == 0 && content.starts_with("- ")) {
            if (active && !current.chart_path.empty()) charts.push_back(current);
            current = {};
            active = true;
            content = Trim(content.substr(2));
        }
        if (!active || (indent != std::string::npos && indent > 2)) continue;
        const size_t colon = content.find(':');
        if (colon == std::string::npos) continue;
        const std::string key = Trim(content.substr(0, colon));
        const std::string value = Trim(content.substr(colon + 1));
        if (key == "chartPath") current.chart_path = value;
        else if (key == "audioPath") current.audio_path = value;
        else if (key == "jacketPath") current.jacket_path = value;
        else if (key == "backgroundPath") current.background_path = value;
        else if (key == "title") current.title = value;
        else if (key == "composer") current.composer = value;
        else if (key == "charter") current.charter = value;
        else if (key == "illustrator") current.illustrator = value;
        else if (key == "difficulty") current.difficulty = value;
        else if (key == "bpmText") current.bpm_text = value;
        else if (key == "baseBpm") current.base_bpm = std::max(1.0, std::strtod(value.c_str(), nullptr));
        else if (key == "chartConstant") current.chart_constant = std::strtod(value.c_str(), nullptr);
        else if (key == "previewStart") current.preview_start = std::strtoll(value.c_str(), nullptr, 10);
        else if (key == "previewEnd") current.preview_end = std::strtoll(value.c_str(), nullptr, 10);
    }
    if (active && !current.chart_path.empty()) charts.push_back(current);
    return charts;
}

std::string JoinZipPath(std::string_view a, std::string_view b) {
    std::string raw(a);
    if (!raw.empty() && raw.back() != '/') raw.push_back('/');
    raw += b;
    std::string normalized;
    return Archive::NormalizePath(raw, normalized) ? normalized : std::string{};
}

const Json *JsonFind(const Json *object, std::string_view key) {
    if (!object || !object->is_object()) return nullptr;
    const auto value = object->find(std::string(key));
    return value == object->end() ? nullptr : &*value;
}

std::string JsonString(const Json *value, std::string fallback = {}) {
    if (value && value->is_string()) return value->get_ref<const std::string &>();
    return fallback;
}

double JsonNumber(const Json *value, double fallback) {
    if (value && value->is_number()) return value->get<double>();
    return fallback;
}

bool JsonBool(const Json *value, bool fallback) {
    if (value && value->is_boolean()) return value->get<bool>();
    return fallback;
}

const Entry *FindRawAudio(const Archive &archive, std::string_view prefix) {
    for (const char *name : {"base.ogg", "base.wav"}) {
        if (!prefix.empty()) {
            if (const auto *e = FindCaseInsensitive(archive, std::string(prefix) + name)) return e;
        }
        if (const auto *e = FindCaseInsensitive(archive, name)) return e;
    }
    return FindOneByExtension(archive, {".ogg", ".wav"}, prefix);
}

const Entry *FindRawJacket(const Archive &archive, std::string_view prefix) {
    for (const char *name : {"base.jpg", "base.png", "base.jpeg"}) {
        if (!prefix.empty()) if (const auto *e = FindCaseInsensitive(archive, std::string(prefix) + name)) return e;
        if (const auto *e = FindCaseInsensitive(archive, name)) return e;
    }
    const Entry *candidate = nullptr;
    for (const auto &entry : archive.Entries()) {
        if (!prefix.empty() && !entry.name.starts_with(prefix)) continue;
        const std::string ext = Extension(entry.name);
        const std::string base = Lower(BaseName(entry.name));
        if (entry.directory || (ext != ".jpg" && ext != ".jpeg" && ext != ".png")) continue;
        if (base.find("_256") != std::string::npos) continue;
        if (base.find("base") == std::string::npos) continue;
        if (candidate) return nullptr;
        candidate = &entry;
    }
    return candidate;
}

const Entry *FindRawBackground(const Archive &archive, std::string_view prefix, std::string_view bg) {
    if (bg.empty()) return nullptr;
    const std::string wanted = std::string(bg);
    for (const char *ext : {".jpg", ".jpeg", ".png"}) {
        if (!prefix.empty()) if (const auto *e = FindCaseInsensitive(archive, std::string(prefix) + wanted + ext)) return e;
        if (const auto *e = FindCaseInsensitive(archive, wanted + ext)) return e;
    }
    return nullptr;
}

void SetRatingFromConstant(ImportedChart &chart, double constant) {
    if (!std::isfinite(constant) || constant < 0.0) return;
    chart.rating = static_cast<int>(std::floor(constant + 1e-6));
    chart.rating_plus = constant - static_cast<double>(chart.rating) >= 0.5 - 1e-6;
}

} // namespace

CustomChartManager &CustomChartManager::Instance() {
    static CustomChartManager manager;
    return manager;
}

void CustomChartManager::AddDiagnostic(std::string package, std::string item,
                                       std::string status, std::string detail) {
    diagnostics_.push_back({std::move(package), std::move(item), std::move(status), std::move(detail)});
}

bool CustomChartManager::EnsureInstalled(const cfg::GameProfile &profile) {
    if (ready_) return hooks_installed_;
    profile_ = profile;
    RuntimeConfig::Instance().EnsureLoaded();
    if (!RuntimeConfig::Instance().CustomChartsEnabled() || !profile.capabilities.custom_charts) {
        ARC_LOGI("CustomCharts: disabled or unavailable for %s", profile.version_name);
        ready_ = true;
        return false;
    }
    ImportAll();
    if (!songs_.empty()) hooks_installed_ = AssetVirtualizer::Instance().Install(profile);
    ready_ = true;
    ARC_LOGI("CustomCharts: songs=%zu assets=%zu hooks=%s", songs_.size(), assets_.size(),
             hooks_installed_ ? "OK" : "FAILED");
    return hooks_installed_;
}

bool CustomChartManager::ImportAll() {
    songs_.clear(); assets_.clear(); diagnostics_.clear();
    const auto &runtime = RuntimeConfig::Instance();
    std::error_code ec;
    std::filesystem::create_directories(runtime.ChartsDir(), ec);
    ec.clear();
    std::filesystem::create_directories(runtime.CacheDir(), ec);
    std::vector<std::filesystem::path> packages;
    for (std::filesystem::directory_iterator it(runtime.ChartsDir(), ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_regular_file()) continue;
        const std::string ext = Lower(it->path().extension().string());
        if (ext == ".arcpkg" || ext == ".zip") packages.push_back(it->path());
    }
    std::sort(packages.begin(), packages.end());
    std::vector<std::string> active_hashes;
    for (const auto &path : packages) {
        std::string hash_error;
        const std::string hash = crypto::Sha256FileHex(path.string(), &hash_error);
        if (hash.empty()) {
            AddDiagnostic(path.filename().string(), "package", "SKIPPED_SONG", "sha256: " + hash_error);
            continue;
        }
        active_hashes.push_back(hash);
        ImportPackage(path.string(), hash);
    }
    for (const auto &song : songs_) RegisterSongAssets(song);
    WriteReports(active_hashes);

    std::set<std::string> keep(active_hashes.begin(), active_hashes.end());
    ec.clear();
    for (std::filesystem::directory_iterator it(runtime.CacheDir(), ec), end; !ec && it != end; it.increment(ec)) {
        if (!it->is_directory() || keep.contains(it->path().filename().string())) continue;
        std::filesystem::remove_all(it->path(), ec);
        ec.clear();
    }
    return true;
}

bool CustomChartManager::ImportPackage(const std::string &path, const std::string &hash) {
    const std::string ext = Lower(std::filesystem::path(path).extension().string());
    return ext == ".arcpkg" ? ImportArcPackage(path, hash) : ImportRawZip(path, hash);
}

bool CustomChartManager::ImportArcPackage(const std::string &path, const std::string &hash) {
    Archive archive;
    std::string error;
    const std::string package_name = std::filesystem::path(path).filename().string();
    if (!archive.Open(path, error)) {
        AddDiagnostic(package_name, "package", "SKIPPED_SONG", error);
        return false;
    }
    std::string index_text;
    if (!ReadEntryText(archive, FindCaseInsensitive(archive, "index.yml"), index_text, error)) {
        AddDiagnostic(package_name, "index.yml", "SKIPPED_SONG", error);
        return false;
    }
    const auto index = ParseArcIndex(index_text);
    size_t ordinal = 0;
    for (const auto &item : index) {
        ++ordinal;
        if (!item.type.empty() && item.type != "level") continue;
        const std::string settings_path = JoinZipPath(item.directory, item.settings_file);
        std::string project_text;
        if (settings_path.empty() || !ReadEntryText(archive, FindCaseInsensitive(archive, settings_path), project_text, error)) {
            AddDiagnostic(package_name, item.identifier, "SKIPPED_SONG", "project: " + error);
            continue;
        }
        const auto settings = ParseArcProject(project_text);
        ImportedSong song;
        song.source_id = item.identifier.empty() ? item.directory : item.identifier;
        song.id = MakeSongId(song.source_id, hash, ordinal > 1 ? ordinal : 0);
        const std::string cache_base = RuntimeConfig::Instance().CacheDir() + "/" + hash + "/" + song.id;
        const Entry *audio_entry = nullptr;
        const Entry *jacket_entry = nullptr;
        const Entry *background_entry = nullptr;
        for (const auto &chart : settings) {
            const auto slot = SlotFromPathOrDifficulty(chart.chart_path, chart.difficulty);
            if (!slot || *slot < 0 || *slot > 3) {
                AddDiagnostic(package_name, chart.chart_path, "SKIPPED_CHART", "unmapped difficulty slot");
                continue;
            }
            const std::string chart_zip = JoinZipPath(item.directory, chart.chart_path);
            const Entry *chart_entry = FindCaseInsensitive(archive, chart_zip);
            if (!chart_entry) {
                AddDiagnostic(package_name, chart.chart_path, "SKIPPED_CHART", "AFF missing");
                continue;
            }
            if (!audio_entry) audio_entry = FindCaseInsensitive(archive, JoinZipPath(item.directory, chart.audio_path));
            if (!jacket_entry && !chart.jacket_path.empty())
                jacket_entry = FindCaseInsensitive(archive, JoinZipPath(item.directory, chart.jacket_path));
            if (!background_entry && !chart.background_path.empty()) {
                background_entry = FindCaseInsensitive(archive, JoinZipPath(item.directory, chart.background_path));
            }
            if (!audio_entry) {
                AddDiagnostic(package_name, chart.chart_path, "SKIPPED_CHART", "audio missing");
                continue;
            }
            const std::string chart_out = cache_base + "/" + std::to_string(*slot) + ".aff";
            if (!archive.ExtractToFile(*chart_entry, chart_out, error)) {
                AddDiagnostic(package_name, chart.chart_path, "SKIPPED_CHART", error);
                continue;
            }
            auto &out = song.charts[*slot];
            out.slot = *slot; out.chart_path = chart_out; out.source_name = chart.chart_path;
            out.charter = chart.charter.empty() ? "Unknown" : chart.charter;
            out.jacket_designer = chart.illustrator.empty() ? "Unknown" : chart.illustrator;
            SetRatingFromConstant(out, chart.chart_constant);
            song.has_chart[*slot] = true;
            if (song.title.empty()) song.title = chart.title;
            if (song.artist == "Unknown" && !chart.composer.empty()) song.artist = chart.composer;
            if (chart.base_bpm > 0) song.bpm_base = chart.base_bpm;
            if (!chart.bpm_text.empty()) song.bpm = chart.bpm_text;
            song.preview_start = chart.preview_start;
            song.preview_end = chart.preview_end > chart.preview_start ? chart.preview_end : chart.preview_start + 30000;
        }
        if (std::none_of(song.has_chart.begin(), song.has_chart.end(), [](bool v) { return v; }) || !audio_entry) {
            AddDiagnostic(package_name, song.source_id, "SKIPPED_SONG", "no playable chart");
            continue;
        }
        song.title = song.title.empty() ? item.directory : song.title;
        if (song.bpm.empty()) song.bpm = FormatBpm(song.bpm_base);
        song.audio_path = cache_base + "/base" + Extension(audio_entry->name);
        if (!archive.ExtractToFile(*audio_entry, song.audio_path, error)) {
            AddDiagnostic(package_name, song.source_id, "SKIPPED_SONG", "audio: " + error);
            continue;
        }
        if (jacket_entry) {
            song.jacket_path = cache_base + "/jacket" + Extension(jacket_entry->name);
            if (!archive.ExtractToFile(*jacket_entry, song.jacket_path, error)) song.jacket_path.clear();
            song.jacket_256_path = song.jacket_path;
        }
        if (song.jacket_path.empty()) {
            song.jacket_path = "@official:img/default_jacket.jpg";
            song.jacket_256_path = "@official:img/default_jacket_256.jpg";
            AddDiagnostic(package_name, song.source_id, "DEFAULTED_FIELD", "jacket");
        }
        if (background_entry) {
            song.bg = "ahbg_" + song.id;
            song.bg_path = cache_base + "/bg" + Extension(background_entry->name);
            if (!archive.ExtractToFile(*background_entry, song.bg_path, error)) {
                song.bg_path.clear();
                song.bg = "base_conflict";
            }
        }
        songs_.push_back(std::move(song));
        AddDiagnostic(package_name, songs_.back().id, "LOADED", "arcpkg");
    }
    return true;
}

bool CustomChartManager::ImportRawZip(const std::string &path, const std::string &hash) {
    Archive archive;
    std::string error;
    const std::string package_name = std::filesystem::path(path).filename().string();
    if (!archive.Open(path, error)) {
        AddDiagnostic(package_name, "package", "SKIPPED_SONG", error);
        return false;
    }
    Json metadata;
    bool metadata_ok = false;
    if (const Entry *songlist = FindCaseInsensitive(archive, "songlist.json")) {
        std::string text;
        if (ReadEntryText(archive, songlist, text, error)) {
            metadata = Json::parse(text, nullptr, false);
            const Json *songs = JsonFind(&metadata, "songs");
            metadata_ok = !metadata.is_discarded() && songs && songs->is_array();
        }
        if (!metadata_ok) AddDiagnostic(package_name, "songlist.json", "DEFAULTED_FIELD", "malformed; fallback discovery");
    }

    struct RawCandidate {
        const Json *metadata = nullptr;
        std::string prefix;
        std::string fallback_id;
    };
    std::vector<RawCandidate> candidates;
    if (metadata_ok) {
        size_t metadata_index = 0;
        for (const auto &value : *JsonFind(&metadata, "songs")) {
            ++metadata_index;
            if (value.is_object()) {
                candidates.push_back({&value, {}, Stem(package_name)});
            } else {
                AddDiagnostic(package_name, "songs[" + std::to_string(metadata_index - 1) + "]",
                              "SKIPPED_SONG", "metadata entry is not an object");
            }
        }
    } else {
        // Metadata-free fallback groups AFF files by their containing directory.
        // This keeps damage isolated when a ZIP contains several independent
        // chart folders instead of collapsing the entire package into one item.
        std::set<std::string> prefixes;
        for (const auto &entry : archive.Entries()) {
            if (entry.directory || Extension(entry.name) != ".aff") continue;
            const size_t slash = entry.name.find_last_of('/');
            prefixes.insert(slash == std::string::npos ? std::string{} : entry.name.substr(0, slash + 1));
        }
        for (const auto &prefix : prefixes) {
            if (!FindRawAudio(archive, prefix)) continue;
            std::string fallback_id = Stem(package_name);
            if (!prefix.empty()) {
                const std::string without_slash = prefix.substr(0, prefix.size() - 1);
                fallback_id = BaseName(without_slash);
            }
            candidates.push_back({nullptr, prefix, std::move(fallback_id)});
        }
    }
    if (candidates.empty()) candidates.push_back({nullptr, {}, Stem(package_name)});

    size_t ordinal = 0;
    for (const auto &candidate : candidates) {
        const Json *value = candidate.metadata;
        ++ordinal;
        ImportedSong song;
        song.source_id = value ? JsonString(JsonFind(value, "id"), candidate.fallback_id) : candidate.fallback_id;
        song.id = MakeSongId(song.source_id, hash, ordinal > 1 ? ordinal : 0);
        std::string prefix = candidate.prefix;
        if (value) {
            const std::string metadata_prefix = song.source_id + "/";
            if (FindRawAudio(archive, metadata_prefix)) prefix = metadata_prefix;
        }
        const Entry *audio = FindRawAudio(archive, prefix);
        if (!audio) {
            AddDiagnostic(package_name, song.source_id, "SKIPPED_SONG", "audio missing or ambiguous");
            continue;
        }
        song.title = Stem(package_name);
        if (value) {
            if (const auto *localized = JsonFind(value, "title_localized")) {
                if (const auto *en = JsonFind(localized, "en")) song.title = JsonString(en, song.title);
                else if (localized->is_object() && !localized->empty())
                    song.title = JsonString(&localized->begin().value(), song.title);
            }
            song.artist = JsonString(JsonFind(value, "artist"), "Unknown");
            song.bpm = JsonString(JsonFind(value, "bpm"), "120");
            song.bpm_base = JsonNumber(JsonFind(value, "bpm_base"), 120.0);
            song.side = static_cast<int>(JsonNumber(JsonFind(value, "side"), 1));
            song.bg = JsonString(JsonFind(value, "bg"), "base_conflict");
            song.preview_start = static_cast<int64_t>(JsonNumber(JsonFind(value, "audioPreview"), 0));
            song.preview_end = static_cast<int64_t>(JsonNumber(JsonFind(value, "audioPreviewEnd"), song.preview_start + 30000));
        }
        const std::string cache_base = RuntimeConfig::Instance().CacheDir() + "/" + hash + "/" + song.id;
        const Entry *background = FindRawBackground(archive, prefix, song.bg);

        std::map<int, const Json *> difficulty_meta;
        if (value) if (const auto *difficulties = JsonFind(value, "difficulties"); difficulties && difficulties->is_array()) {
            for (const auto &difficulty : *difficulties) {
                const int slot = static_cast<int>(JsonNumber(JsonFind(&difficulty, "ratingClass"), -1));
                if (slot >= 0 && slot <= 3) difficulty_meta[slot] = &difficulty;
            }
        }
        std::vector<std::pair<int, const Entry *>> charts;
        for (int slot = 0; slot <= 3; ++slot) {
            const Entry *entry = nullptr;
            if (!prefix.empty()) entry = FindCaseInsensitive(archive, prefix + std::to_string(slot) + ".aff");
            if (!entry) entry = FindCaseInsensitive(archive, std::to_string(slot) + ".aff");
            if (entry) charts.emplace_back(slot, entry);
        }
        if (charts.empty()) {
            const Entry *only = FindOneByExtension(archive, {".aff"}, prefix);
            if (!only && !prefix.empty()) only = FindOneByExtension(archive, {".aff"});
            if (only) charts.emplace_back(2, only);
        }
        for (const auto &[slot, entry] : charts) {
            const std::string chart_out = cache_base + "/" + std::to_string(slot) + ".aff";
            if (!archive.ExtractToFile(*entry, chart_out, error)) {
                AddDiagnostic(package_name, entry->name, "SKIPPED_CHART", error);
                continue;
            }
            auto &chart = song.charts[slot];
            chart.slot = slot; chart.chart_path = chart_out; chart.source_name = entry->name;
            if (const auto it = difficulty_meta.find(slot); it != difficulty_meta.end()) {
                chart.charter = JsonString(JsonFind(it->second, "chartDesigner"), "Unknown");
                chart.jacket_designer = JsonString(JsonFind(it->second, "jacketDesigner"), "Unknown");
                chart.rating = static_cast<int>(JsonNumber(JsonFind(it->second, "rating"), 0));
                chart.rating_plus = JsonBool(JsonFind(it->second, "ratingPlus"), false);
            }
            song.has_chart[slot] = true;
            if (!metadata_ok) {
                std::string aff;
                if (ReadEntryText(archive, entry, aff, error)) {
                    song.bpm_base = FirstAffBpm(aff);
                    song.bpm = FormatBpm(song.bpm_base);
                }
            }
        }
        if (std::none_of(song.has_chart.begin(), song.has_chart.end(), [](bool v) { return v; })) {
            AddDiagnostic(package_name, song.source_id, "SKIPPED_SONG", "chart missing or ambiguous");
            continue;
        }
        song.audio_path = cache_base + "/base" + Extension(audio->name);
        if (!archive.ExtractToFile(*audio, song.audio_path, error)) {
            AddDiagnostic(package_name, song.source_id, "SKIPPED_SONG", "audio: " + error);
            continue;
        }
        const Entry *jacket = FindRawJacket(archive, prefix);
        if (jacket) {
            song.jacket_path = cache_base + "/jacket" + Extension(jacket->name);
            if (!archive.ExtractToFile(*jacket, song.jacket_path, error)) song.jacket_path.clear();
            song.jacket_256_path = song.jacket_path;
        }
        if (song.jacket_path.empty()) {
            song.jacket_path = "@official:img/default_jacket.jpg";
            song.jacket_256_path = "@official:img/default_jacket_256.jpg";
            AddDiagnostic(package_name, song.source_id, "DEFAULTED_FIELD", "jacket");
        }
        if (background) {
            song.bg = "ahbg_" + song.id;
            song.bg_path = cache_base + "/bg" + Extension(background->name);
            if (!archive.ExtractToFile(*background, song.bg_path, error)) {
                song.bg_path.clear();
                song.bg = "base_conflict";
            }
        }
        if (!value) {
            song.side = 1; song.bg = "base_conflict";
            AddDiagnostic(package_name, song.source_id, "DEFAULTED_FIELD", "metadata side=1 bg=base_conflict");
        }
        songs_.push_back(std::move(song));
        AddDiagnostic(package_name, songs_.back().id, "LOADED", metadata_ok ? "raw zip" : "raw zip fallback");
    }
    return true;
}

void CustomChartManager::RegisterSongAssets(const ImportedSong &song) {
    const std::string root = "songs/" + song.id + "/";
    assets_[root + "base.ogg"] = song.audio_path;
    assets_[root + "base.jpg"] = song.jacket_path;
    assets_[root + "base_256.jpg"] = song.jacket_256_path.empty() ? song.jacket_path : song.jacket_256_path;
    if (!song.bg_path.empty()) {
        const std::string bg_root = "img/bg/1080/" + song.bg;
        assets_[bg_root + ".jpg"] = song.bg_path;
        assets_[bg_root + ".png"] = song.bg_path;
    }
    for (int slot = 0; slot <= 3; ++slot) if (song.has_chart[slot]) {
        assets_[root + std::to_string(slot) + ".aff"] = song.charts[slot].chart_path;
    }
}

const std::string *CustomChartManager::ResolveAsset(std::string_view game_path) const {
    const auto it = assets_.find(std::string(game_path));
    return it == assets_.end() ? nullptr : &it->second;
}

bool CustomChartManager::IsCustomChartPath(std::string_view game_path, std::string *song_id) const {
    const auto it = assets_.find(std::string(game_path));
    if (it == assets_.end() || Extension(game_path) != ".aff") return false;
    if (song_id) {
        constexpr std::string_view prefix = "songs/";
        const size_t slash = game_path.find('/', prefix.size());
        *song_id = slash == std::string_view::npos ? std::string{} : std::string(game_path.substr(prefix.size(), slash - prefix.size()));
    }
    return true;
}

std::string CustomChartManager::BuildSongsJson() const {
    Json result = Json::array();
    for (const auto &song : songs_) {
        Json item = {
            {"id", song.id},
            {"title_localized", {{"en", song.title}}},
            {"artist", song.artist},
            {"bpm", song.bpm},
            {"bpm_base", song.bpm_base},
            {"set", "base"},
            {"purchase", ""},
            {"audioPreview", song.preview_start},
            {"audioPreviewEnd", song.preview_end},
            {"side", song.side},
            {"bg", song.bg},
            {"date", 0},
            {"version", "4.0.0"},
            {"difficulties", Json::array()},
        };
        for (int slot = 0; slot <= 3; ++slot) {
            const auto &chart = song.charts[slot];
            Json difficulty = {
                {"ratingClass", slot},
                {"chartDesigner", song.has_chart[slot] ? chart.charter : ""},
                {"jacketDesigner", song.has_chart[slot] ? chart.jacket_designer : ""},
                {"rating", song.has_chart[slot] ? chart.rating : -1},
            };
            if (song.has_chart[slot] && chart.rating_plus) difficulty["ratingPlus"] = true;
            item["difficulties"].push_back(std::move(difficulty));
        }
        result.push_back(std::move(item));
    }
    return result.dump(-1, ' ', false, Json::error_handler_t::replace);
}

std::string CustomChartManager::MergeSonglist(std::string_view official_json, std::string &error) const {
    error.clear();
    Json merged = Json::parse(official_json, nullptr, false);
    if (merged.is_discarded() || !merged.is_object()) {
        error = "official songlist is invalid";
        return {};
    }
    auto songs = merged.find("songs");
    if (songs == merged.end() || !songs->is_array()) {
        error = "songs array missing";
        return {};
    }
    const Json custom = Json::parse(BuildSongsJson(), nullptr, false);
    if (custom.is_discarded() || !custom.is_array()) {
        error = "generated custom songs are invalid";
        return {};
    }
    songs->insert(songs->end(), custom.begin(), custom.end());
    return merged.dump(-1, ' ', false, Json::error_handler_t::replace);
}

void CustomChartManager::WriteReports(const std::vector<std::string> &active_hashes) const {
    const std::string root = RuntimeConfig::Instance().RootDir();
    Json packages = Json::array();
    for (const auto &hash : active_hashes) packages.push_back({{"sha256", hash}});
    const Json manifest = {
        {"version", 1},
        {"packages", std::move(packages)},
        {"songs", songs_.size()},
    };
    AtomicWrite(root + "/manifest.json",
                manifest.dump(2, ' ', false, Json::error_handler_t::replace) + '\n');

    Json entries = Json::array();
    for (const auto &diagnostic : diagnostics_) {
        entries.push_back({
            {"package", diagnostic.package},
            {"item", diagnostic.item},
            {"status", diagnostic.status},
            {"detail", diagnostic.detail},
        });
    }
    const Json report = {
        {"version", 1},
        {"entries", std::move(entries)},
    };
    AtomicWrite(root + "/import-report.json",
                report.dump(2, ' ', false, Json::error_handler_t::replace) + '\n');
}

} // namespace arc_helper
