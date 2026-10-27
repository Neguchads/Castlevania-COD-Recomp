#pragma once
#include <string>
#include <string_view>

namespace dev {
// Busca do menu dev: minusculas e sem acento (UTF-8 latino), para "camera" achar "Câmera".
inline std::string FoldSearchText(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        const auto c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) { out += char(c >= 'A' && c <= 'Z' ? c + 32 : c); continue; }
        if (c != 0xC3 || i + 1 >= text.size()) continue;   // outro UTF-8: some da busca
        const unsigned char low = static_cast<unsigned char>(text[++i]) & ~0x20u;   // maiuscula e minuscula juntas
        if (low >= 0x80 && low <= 0x86) out += 'a';
        else if (low == 0x87) out += 'c';
        else if (low >= 0x88 && low <= 0x8B) out += 'e';
        else if (low >= 0x8C && low <= 0x8F) out += 'i';
        else if (low == 0x91) out += 'n';
        else if (low >= 0x92 && low <= 0x96) out += 'o';
        else if (low >= 0x99 && low <= 0x9C) out += 'u';
    }
    return out;
}
// Teclado na tela da busca: letra/espaco somam, '\b' apaga a ultima, 0 limpa. Respeita o tamanho do buffer.
inline void SearchKeyPress(char* text, size_t size, char key) {
    if (!text || !size) return;
    size_t length = 0;
    while (length + 1 < size && text[length]) ++length;
    text[length] = 0;
    if (key == 0) { text[0] = 0; return; }
    if (key == '\b') { if (length) text[length - 1] = 0; return; }
    if (length + 1 >= size || (key == ' ' && (!length || text[length - 1] == ' '))) return;
    text[length] = key;
    text[length + 1] = 0;
}
// Cada palavra da busca precisa aparecer no texto, em qualquer ordem. Busca vazia nao acha nada.
inline bool MenuSearchMatch(std::string_view text, std::string_view query) {
    const std::string folded = FoldSearchText(text), words = FoldSearchText(query);
    bool any = false;
    for (size_t start = 0; start < words.size();) {
        const size_t end = words.find(' ', start);
        const std::string_view word = std::string_view(words).substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!word.empty()) {
            if (folded.find(word) == std::string::npos) return false;
            any = true;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return any;
}
}
