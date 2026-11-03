#pragma once
#include <iterator>

namespace dev {
// Abas e paginas do menu dev. O numero e a pagina desenhada no switch de dev_menu.cpp (e na busca).
struct MenuTitleEntry {const char* label;int page;};
struct MenuTabEntry {const char* label;MenuTitleEntry titles[12];int count;};
inline constexpr int kMenuPageCount = 22;   // paginas 0..21
inline constexpr MenuTabEntry kTabs[]{
    {"Jogabilidade",{{"Jogo",0},{"Dificuldade",16},{"Todos os inimigos",18},{"Inimigos aleatorios",19},
                     {"Inimigos por tipo",17},{"Drops e itens",20},{"Innocent Devils",21},{"Camera",11},
                     {"Minimapa e HUD",10},{"Saves e sessao",8}},10},
    {"Video e audio",{{"Graficos",2},{"Audio",3}},2},
    {"Controles",{{"Controle",15},{"Teclado e mouse",4}},2},
    {"Cheats",{{"Cheats",6}},1},
    {"Mods",{{"Mods e texturas",5}},1},
    {"Suporte e menu",{{"Relatar bug",1},{"Capturas salvas",13},{"Logs e travamentos",14},{"Diagnostico",12},
                       {"Aparencia do menu",7},{"Perfis",9}},6}};
inline constexpr int kTabCount = int(std::size(kTabs));
// Aba que mostra a pagina, ou -1.
inline constexpr int TabOfPage(int page) {
    for (int tab = 0; tab < kTabCount; ++tab)
        for (int title = 0; title < kTabs[tab].count; ++title)
            if (kTabs[tab].titles[title].page == page) return tab;
    return -1;
}
}
