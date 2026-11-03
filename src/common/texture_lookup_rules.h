#pragma once
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

// Pacotes de textura: indice dos arquivos montado uma vez por carga dos mods (sem consultar o disco por textura)
// e hash estavel de cada textura guest calculado uma vez por quadro (antes: a cada desenho).
namespace texture_lookup {
struct TextureFile {
    uint64_t hash = 0;
    bool dds = false;
};
// "0123456789ABCDEF.dds" ou ".bmp", 16 digitos hex; qualquer outro nome fica fora do pacote
inline std::optional<TextureFile> ParseTextureFile(std::string_view name) {
    if (name.size() != 20 || name[16] != '.') return std::nullopt;
    auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c; };
    const char e0 = lower(name[17]), e1 = lower(name[18]), e2 = lower(name[19]);
    const bool dds = e0 == 'd' && e1 == 'd' && e2 == 's';
    if (!dds && !(e0 == 'b' && e1 == 'm' && e2 == 'p')) return std::nullopt;
    uint64_t hash = 0;
    for (int i = 0; i < 16; i++) {
        const char c = lower(name[size_t(i)]);
        const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
        if (digit < 0) return std::nullopt;
        hash = hash << 4 | uint64_t(digit);
    }
    return TextureFile{hash, dds};
}
// pastas em ordem de prioridade; na mesma pasta o DDS (BC7 na placa) vence o BMP
template <class Path>
class Index {
public:
    void Add(uint64_t hash, bool dds, Path path) {
        // procura antes de mover: o Entry de um try_emplace ja levaria o caminho mesmo sem inserir
        auto it = entries_.find(hash);
        if (it == entries_.end()) entries_.emplace(hash, Entry{std::move(path), folder_, dds});
        else if (it->second.folder == folder_ && dds && !it->second.dds) it->second = Entry{std::move(path), folder_, dds};
    }
    void NextFolder() { folder_++; }
    const Path* Find(uint64_t hash) const {
        auto it = entries_.find(hash);
        return it == entries_.end() ? nullptr : &it->second.path;
    }
    size_t Size() const { return entries_.size(); }

private:
    struct Entry {
        Path path;
        int folder;
        bool dds;
    };
    std::unordered_map<uint64_t, Entry> entries_;
    int folder_ = 0;
};
// valor por chave valido so no quadro em que foi calculado
class FrameMemo {
public:
    static constexpr size_t kMaxEntries = 4096;
    template <class Compute>
    uint64_t Get(uint64_t key, uint64_t frame, Compute&& compute) {
        auto it = entries_.find(key);
        if (it != entries_.end() && it->second.frame == frame) return it->second.value;
        if (it == entries_.end() && entries_.size() >= kMaxEntries) {
            std::erase_if(entries_, [&](const auto& e) { return e.second.frame != frame; });
            if (entries_.size() >= kMaxEntries) entries_.clear();
        }
        const uint64_t value = compute();
        entries_[key] = {frame, value};
        return value;
    }
    size_t Size() const { return entries_.size(); }

private:
    struct Entry {
        uint64_t frame, value;
    };
    std::unordered_map<uint64_t, Entry> entries_;
};
}  // namespace texture_lookup
