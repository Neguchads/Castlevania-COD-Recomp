#pragma once
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include "texture_pack.h"

namespace dev {
// Previa da captura: reduz por media de blocos ate caber em max_width e deixa opaca
// (o alfa do backbuffer nao significa transparencia).
inline TextureImage ShrinkPreview(const TextureImage& image, uint32_t max_width) {
    const uint32_t factor = std::max(1u, (image.width + max_width - 1) / std::max(1u, max_width));
    TextureImage out{std::max(1u, image.width / factor), std::max(1u, image.height / factor), {}};
    out.bgra.resize(size_t(out.width) * out.height * 4);
    for (uint32_t y = 0; y < out.height; ++y)
        for (uint32_t x = 0; x < out.width; ++x) {
            uint32_t sum[3]{}, count = 0;
            for (uint32_t sy = y * factor; sy < std::min(image.height, (y + 1) * factor); ++sy)
                for (uint32_t sx = x * factor; sx < std::min(image.width, (x + 1) * factor); ++sx, ++count)
                    for (int c = 0; c < 3; ++c) sum[c] += image.bgra[(size_t(sy) * image.width + sx) * 4 + size_t(c)];
            uint8_t* pixel = &out.bgra[(size_t(y) * out.width + x) * 4];
            for (int c = 0; c < 3; ++c) pixel[c] = uint8_t(sum[c] / std::max(1u, count));
            pixel[3] = 255;
        }
    return out;
}
// Capturas e relatos salvos em reports/<data>, mais novos primeiro.
struct ReportEntry { std::string folder, title; bool screenshot = false; };
// So os nomes entram na ordenacao; report.txt e a captura sao abertos apenas nas `limit` pastas mostradas
// (a galeria atualiza a cada 2 s na thread do jogo, com centenas de relatos acumulados).
inline std::vector<std::string> NewestFolders(const std::filesystem::path& root, size_t limit) {
    namespace fs = std::filesystem;
    std::vector<std::string> names;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(root, ec))
        if (entry.is_directory(ec)) names.push_back(entry.path().filename().string());
    const size_t keep = std::min(limit, names.size());
    std::partial_sort(names.begin(), names.begin() + std::ptrdiff_t(keep), names.end(), std::greater<>());
    names.resize(keep);
    return names;
}
inline std::vector<ReportEntry> ListReports(const std::filesystem::path& root, size_t limit) {
    namespace fs = std::filesystem;
    std::vector<ReportEntry> out;
    std::error_code ec;
    for (const auto& name : NewestFolders(root, limit)) {
        const fs::path folder = root / name;
        ReportEntry item{name, "(sem relato)", fs::is_regular_file(folder / "screenshot.bmp", ec)};
        std::ifstream text(folder / "report.txt", std::ios::binary);
        for (std::string line; text && std::getline(text, line);) {
            if (line.rfind("Titulo:", 0) != 0) continue;
            line.erase(0, 7);
            line.erase(0, line.find_first_not_of(' ') == std::string::npos ? line.size() : line.find_first_not_of(' '));
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
            item.title = line.empty() ? "(sem titulo)" : line;
            break;
        }
        out.push_back(std::move(item));
    }
    return out;
}
// Ultimas linhas inteiras de um log (le so o final do arquivo).
inline std::string LogTail(const std::filesystem::path& path, size_t lines) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input || !lines) return {};
    const std::streamoff size = input.tellg();
    const std::streamoff start = std::max<std::streamoff>(0, size - std::streamoff(lines) * 512);
    input.seekg(start);
    std::string text(size_t(size - start), '\0');
    input.read(text.data(), std::streamsize(text.size()));
    size_t from = text.size(), count = 0;
    if (!text.empty() && text.back() == '\n') --from;
    while (from > 0 && count < lines) {
        if (text[from - 1] == '\n' && ++count == lines) break;
        --from;
    }
    return text.substr(from);
}
// Travamentos em CrashDumps: o .dmp e, quando existir, o log copiado com o mesmo nome.
struct CrashEntry { std::string name; uintmax_t bytes = 0; bool log = false; };
inline std::vector<CrashEntry> ListCrashes(const std::filesystem::path& root, size_t limit) {
    namespace fs = std::filesystem;
    std::vector<CrashEntry> out;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(root, ec)) {
        if (entry.path().extension() != ".dmp") continue;
        auto log = entry.path();
        log.replace_extension(".log");
        out.push_back({entry.path().stem().string(), entry.file_size(ec), fs::is_regular_file(log, ec)});
    }
    std::sort(out.begin(), out.end(), [](const CrashEntry& a, const CrashEntry& b) { return a.name > b.name; });
    if (out.size() > limit) out.resize(limit);
    return out;
}
}
