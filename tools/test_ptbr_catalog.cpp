// o build publico (COD_NO_PRIVATE_CATALOG) nunca leva o catalogo privado, mesmo com o arquivo presente
#define COD_NO_PRIVATE_CATALOG
#include "../src/common/ptbr_strings.h"
#include "../src/common/ptbr_catalog.h"
#include <cstdio>
#include <filesystem>
#include <fstream>

using namespace cod_ptbr;

int main() {
    // ida e volta: ids fora de ordem, texto vazio, bytes acima de 0x7F e tokens de controle do jogo
    std::map<uint32_t, std::string> in{{7, "Mapa"}, {0, ""}, {1409, "Ol\xC3\xA1\x01\x02 mundo"}, {0xFFFFFFF0u, "fim"}};
    const auto bytes = SerializeCatalog(in);
    std::map<uint32_t, std::string> out;
    if (!ParseCatalog(bytes.data(), bytes.size(), out) || out != in) return 1;
    // arquivo vazio ou de outro formato: nada carregado, sem travar
    out = {{1, "x"}};
    if (ParseCatalog(nullptr, 0, out) || !out.empty()) return 2;
    std::vector<uint8_t> wrong = bytes;
    wrong[0] = 'X';
    if (ParseCatalog(wrong.data(), wrong.size(), out) || !out.empty()) return 3;
    // cortado no meio: recusa inteiro, nao entrega catalogo pela metade
    for (size_t cut = 1; cut < bytes.size(); cut++)
        if (ParseCatalog(bytes.data(), cut, out) || !out.empty()) return 4;
    // contagem ou tamanho mentindo (maior que o arquivo) e bytes sobrando no fim
    std::vector<uint8_t> big = SerializeCatalog({{1, "ab"}});
    big[12 + 4] = 0xFF;  // tamanho do texto da primeira entrada
    if (ParseCatalog(big.data(), big.size(), out)) return 5;
    std::vector<uint8_t> count = SerializeCatalog({{1, "ab"}});
    count[8] = 0xFF;
    if (ParseCatalog(count.data(), count.size(), out)) return 6;
    std::vector<uint8_t> tail = bytes;
    tail.push_back(0);
    if (ParseCatalog(tail.data(), tail.size(), out)) return 7;
    // texto com byte zero no meio cortaria a string do jogo: recusado
    const auto zero = SerializeCatalog({{2, std::string("a\0b", 3)}});
    if (ParseCatalog(zero.data(), zero.size(), out)) return 8;
    if (!translations.empty()) return 9;
    // bytes fixos, os mesmos de tools/test_release_kit.py: C++ e Python leem o mesmo formato
    const std::vector<uint8_t> golden{0x43, 0x4F, 0x44, 0x50, 0x54, 0x42, 0x52, 0x31, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0,
                                      'a', 'b', 7, 0, 0, 0, 4, 0, 0, 0, 'M', 'a', 'p', 'a'};
    const std::map<uint32_t, std::string> small{{1, "ab"}, {7, "Mapa"}};
    if (SerializeCatalog(small) != golden) return 10;
    // arquivo: ausente, invalido e valido, como o jogo le de mods/PTBR-PS2
    const auto dir = std::filesystem::temp_directory_path() / "cod_ptbr_catalog_test";
    std::filesystem::create_directories(dir);
    const auto file = dir / kCatalogFile;
    std::filesystem::remove(file);
    out = {{1, "x"}};
    if (LoadCatalogFile(file, out) != CatalogLoad::Missing || !out.empty()) return 11;
    std::ofstream(file, std::ios::binary).write("lixo", 4);
    if (LoadCatalogFile(file, out) != CatalogLoad::Invalid || !out.empty()) return 12;
    std::ofstream(file, std::ios::binary).write(reinterpret_cast<const char*>(golden.data()), std::streamsize(golden.size()));
    if (LoadCatalogFile(file, out) != CatalogLoad::Loaded || out != small) return 13;
    std::filesystem::remove_all(dir);
    std::puts("PT-BR catalog: ida e volta, vazio, formato errado, cortado, tamanhos falsos e zero no texto, bytes fixos, arquivo e build publico sem catalogo");
}
