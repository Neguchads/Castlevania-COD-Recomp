#include <cstdio>
#include <cstring>
#include "../src/common/menu_search.h"
#include "../src/common/menu_search_index.h"

using namespace dev;

int main() {
    if (FoldSearchText("Câmera ÁÉÍÓÚ ção") != "camera aeiou cao") return 1;
    if (FoldSearchText("MiXeD 123") != "mixed 123") return 2;
    if (!MenuSearchMatch("Inverter camera horizontal", "CÂMERA")) return 3;
    if (!MenuSearchMatch("Vida dos inimigos", "inimigos vida")) return 4;
    if (MenuSearchMatch("Vida dos inimigos", "vida mana")) return 5;
    if (MenuSearchMatch("Vida dos inimigos", "") || MenuSearchMatch("Vida dos inimigos", "   ")) return 6;
    if (!MenuSearchMatch("Tamanho dos inimigos", "  tam  ")) return 7;
    // Teclado na tela: soma, espaco unico, apaga, limpa e nao estoura o buffer.
    char text[5]{};
    SearchKeyPress(text, sizeof(text), ' ');
    SearchKeyPress(text, sizeof(text), 'v');SearchKeyPress(text, sizeof(text), ' ');SearchKeyPress(text, sizeof(text), ' ');
    SearchKeyPress(text, sizeof(text), 'i');
    if (std::strcmp(text, "v i") != 0) return 10;
    SearchKeyPress(text, sizeof(text), 'd');SearchKeyPress(text, sizeof(text), 'a');
    if (std::strcmp(text, "v id") != 0) return 11;
    SearchKeyPress(text, sizeof(text), '\b');
    if (std::strcmp(text, "v i") != 0) return 12;
    SearchKeyPress(text, sizeof(text), 0);
    if (text[0] != 0) return 13;
    // Indice gerado: tem as opcoes e todas apontam para paginas validas do menu.
    bool hp = false, camera = false;
    for (const auto& entry : kMenuSearchIndex) {
        if (entry.page < 0 || entry.page > 64 || !entry.label || !*entry.label) return 8;
        if (std::strcmp(entry.label, "Vida dos inimigos") == 0 && entry.page == 18) hp = true;
        if (std::strcmp(entry.label, "Inverter camera horizontal (analogico direito)") == 0 && entry.page == 15) camera = true;
    }
    if (!hp || !camera) return 9;
    std::puts("Menu search: accent and case folding, all words, empty query and generated index passed");
}
