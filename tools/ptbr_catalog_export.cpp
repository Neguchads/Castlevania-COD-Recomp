// Gera mods/PTBR-PS2/ptbr_strings.bin a partir do catalogo privado (assets/ptbr_strings.private.h).
// So roda na maquina que tem o catalogo; o .bin vai no pacote PT-BR, nunca no repositorio.
// Uso: ptbr_catalog_export.exe <saida.bin>
#include "../src/common/ptbr_strings.h"
#include "../src/common/ptbr_catalog.h"
#include <cstdio>
#include <fstream>
#include <iterator>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fputs("uso: ptbr_catalog_export <saida.bin>\n", stderr);
        return 2;
    }
    if (cod_ptbr::translations.empty()) {
        std::fputs("catalogo privado ausente: assets/ptbr_strings.private.h\n", stderr);
        return 3;
    }
    std::map<uint32_t, std::string> catalog;
    for (const auto& [id, text] : cod_ptbr::translations) catalog.emplace(id, text);
    const auto bytes = cod_ptbr::SerializeCatalog(catalog);
    {
        std::ofstream out(argv[1], std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        if (!out) {
            std::fputs("falha ao gravar\n", stderr);
            return 4;
        }
    }
    // relê o arquivo gravado: o jogo vai ler exatamente estes bytes
    std::ifstream in(argv[1], std::ios::binary);
    const std::vector<uint8_t> back{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
    std::map<uint32_t, std::string> parsed;
    if (!cod_ptbr::ParseCatalog(back.data(), back.size(), parsed) || parsed != catalog) {
        std::fputs("arquivo gravado nao confere com o catalogo\n", stderr);
        return 5;
    }
    std::printf("ptbr_strings.bin: %u textos, %u bytes\n", unsigned(catalog.size()), unsigned(back.size()));
}
