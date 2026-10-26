#pragma once
#include <iterator>

namespace dev {
// Abas e paginas do menu dev. O numero e a pagina desenhada no switch de dev_menu.cpp (e na busca).
struct MenuTitleEntry {const char* label;int page;};
struct MenuTabEntry {const char* label;MenuTitleEntry titles[12];int count;};
inline constexpr int kMenuPageCount = 22;   // paginas 0..21
inline constexpr MenuTabEntry kTabs[]{
    {"Jogabilidade",{{"Jogo",0},{"Dificuldade",16},{"Saves e sessao",8}},3},
    {"Inimigos",{{"Todos os inimigos",18},{"Aleatorio",19},{"Por tipo",17}},3},
    {"Drops e itens",{{"Drops e itens",20}},1},
    {"Pets",{{"Innocent Devils",21}},1},
    {"Graficos",{{"Graficos",2},{"Camera",11},{"Minimapa e HUD",10}},3},
    {"Audio",{{"Audio",3}},1},
    {"Controles",{{"Controle",15},{"Teclado e mouse",4}},2},
    {"Cheats",{{"Cheats",6}},1},
    {"Mods",{{"Mods e texturas",5}},1},
    {"Suporte",{{"Relatar bug",1},{"Capturas salvas",13},{"Logs e travamentos",14},{"Diagnostico",12}},4},
    {"Menu",{{"Aparencia",7},{"Perfis",9}},2}};
inline constexpr int kTabCount = int(std::size(kTabs));
// Aba que mostra a pagina, ou -1.
inline constexpr int TabOfPage(int page) {
    for (int tab = 0; tab < kTabCount; ++tab)
        for (int title = 0; title < kTabs[tab].count; ++title)
            if (kTabs[tab].titles[title].page == page) return tab;
    return -1;
}
}
