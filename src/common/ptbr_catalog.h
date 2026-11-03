#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <vector>

// Catalogo de textos PT-BR em arquivo, dentro do pacote mods/PTBR-PS2 (ptbr_strings.bin).
// O exe publico nao leva os textos (derivados do jogo e do patch dos fas); eles chegam com o pacote.
// Formato, tudo little-endian: "CODPTBR1", u32 quantidade, e por entrada u32 id, u32 tamanho, bytes (sem zero).
namespace cod_ptbr {
inline constexpr char kCatalogMagic[8] = {'C', 'O', 'D', 'P', 'T', 'B', 'R', '1'};
inline constexpr const char* kCatalogFile = "ptbr_strings.bin";

inline std::vector<uint8_t> SerializeCatalog(const std::map<uint32_t, std::string>& catalog) {
    std::vector<uint8_t> out(kCatalogMagic, kCatalogMagic + 8);
    auto put = [&](uint32_t v) { for (int i = 0; i < 4; i++) out.push_back(uint8_t(v >> (8 * i))); };
    put(uint32_t(catalog.size()));
    for (const auto& [id, text] : catalog) {
        put(id);
        put(uint32_t(text.size()));
        out.insert(out.end(), text.begin(), text.end());
    }
    return out;
}

// Falso (e catalogo vazio) se o arquivo nao for exatamente um catalogo valido.
inline bool ParseCatalog(const uint8_t* data, size_t size, std::map<uint32_t, std::string>& out) {
    out.clear();
    size_t at = 0;
    auto get = [&](uint32_t& v) {
        if (size - at < 4) return false;
        v = uint32_t(data[at]) | uint32_t(data[at + 1]) << 8 | uint32_t(data[at + 2]) << 16 | uint32_t(data[at + 3]) << 24;
        at += 4;
        return true;
    };
    uint32_t count = 0;
    if (!data || size < 12 || std::memcmp(data, kCatalogMagic, 8) != 0) return false;
    at = 8;
    if (!get(count) || count > (size - at) / 8) return false;
    std::map<uint32_t, std::string> catalog;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t id = 0, length = 0;
        if (!get(id) || !get(length) || length > size - at) return false;
        std::string text(reinterpret_cast<const char*>(data + at), length);
        if (text.find('\0') != std::string::npos) return false;
        at += length;
        catalog[id] = std::move(text);
    }
    if (at != size) return false;
    out = std::move(catalog);
    return true;
}

enum class CatalogLoad { Missing, Invalid, Loaded };

inline CatalogLoad LoadCatalogFile(const std::filesystem::path& path, std::map<uint32_t, std::string>& out) {
    out.clear();
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return CatalogLoad::Missing;
    const std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return ParseCatalog(bytes.data(), bytes.size(), out) ? CatalogLoad::Loaded : CatalogLoad::Invalid;
}
}
