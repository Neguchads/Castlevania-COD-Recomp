#include "../src/common/report_gallery.h"
#include <cstdio>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    const fs::path root = fs::temp_directory_path() / "cod-report-gallery-test";
    std::error_code ec;
    fs::remove_all(root, ec);
    auto write = [](const fs::path& path, const std::string& text) {
        fs::create_directories(path.parent_path());
        std::ofstream(path, std::ios::binary) << text;
    };
    // Relatos: o mais novo primeiro (nome com data), titulo lido do report.txt, captura opcional.
    write(root / "reports/20261025-101500-1/report.txt", "Castlevania\n\nTitulo: Chao some na sala\n\nRelato:\nx\n");
    write(root / "reports/20261025-101500-1/screenshot.bmp", "BM");
    write(root / "reports/20261025-120000-2/report.txt", "Titulo: \nRelato:\n");
    fs::create_directories(root / "reports/20261024-090000-3");
    write(root / "reports/arquivo-solto.txt", "nao e pasta");
    const auto reports = dev::ListReports(root / "reports", 10);
    if (reports.size() != 3 || reports[0].folder != "20261025-120000-2" || reports[2].folder != "20261024-090000-3") return 1;
    if (reports[1].title != "Chao some na sala" || !reports[1].screenshot || reports[0].screenshot) return 2;
    if (reports[0].title != "(sem titulo)" || reports[2].title != "(sem relato)") return 3;
    if (dev::ListReports(root / "reports", 2).size() != 2 || !dev::ListReports(root / "nao-existe", 5).empty()) return 4;
    // Final do log: so as ultimas linhas, sem quebrar no meio.
    std::string log;
    for (int i = 1; i <= 50; ++i) log += "linha " + std::to_string(i) + "\n";
    write(root / "runtime.log", log);
    const auto tail = dev::LogTail(root / "runtime.log", 3);
    if (tail != "linha 48\nlinha 49\nlinha 50\n") return 5;
    if (!dev::LogTail(root / "nada.log", 3).empty()) return 6;
    // Travamentos: dump e log com o mesmo nome, mais novo primeiro.
    write(root / "CrashDumps/CastlevaniaCOD.2026.10.25_10.00.00.dmp", "MDMP");
    write(root / "CrashDumps/CastlevaniaCOD.2026.10.25_10.00.00.log", "exception C0000005\n");
    write(root / "CrashDumps/CastlevaniaCOD.2026.10.25_11.00.00.dmp", "MDMP");
    const auto crashes = dev::ListCrashes(root / "CrashDumps", 10);
    if (crashes.size() != 2 || crashes[0].name != "CastlevaniaCOD.2026.10.25_11.00.00" || crashes[0].log ||
        !crashes[1].log || crashes[1].bytes != 4) return 7;
    fs::remove_all(root, ec);
    // Previa: 4x2 vira 2x1 com media de blocos 2x2 e alfa opaco; imagem pequena fica do mesmo tamanho.
    dev::TextureImage image{4, 2, std::vector<uint8_t>(4 * 2 * 4, 0)};
    for (uint32_t y = 0; y < 2; ++y)
        for (uint32_t x = 0; x < 2; ++x) image.bgra[(y * 4 + x) * 4] = uint8_t(40 * (y * 2 + x));  // azul 0,40,80,120
    const auto small = dev::ShrinkPreview(image, 2);
    if (small.width != 2 || small.height != 1 || small.bgra.size() != 8 || small.bgra[0] != 60 || small.bgra[3] != 255 ||
        small.bgra[4] != 0) return 8;
    const auto same = dev::ShrinkPreview(image, 1280);
    if (same.width != 4 || same.height != 2 || same.bgra[4] != 40 || same.bgra[7] != 255) return 9;
    std::puts("Report gallery: newest first, titles, screenshots, log tail, crash list and preview shrink passed");
}
