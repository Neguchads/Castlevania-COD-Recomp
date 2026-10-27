#include <cstdio>
#include <set>
#include "../src/common/menu_layout.h"
#include "../src/common/menu_defaults.h"

using namespace dev;

int main() {
    // Cada pagina 0..21 aparece em exatamente uma aba; abas sem titulo vazio.
    for (int page = 0; page < kMenuPageCount; ++page) {
        int seen = 0;
        for (const auto& tab : kTabs)
            for (int i = 0; i < tab.count; ++i) seen += tab.titles[i].page == page;
        if (seen != 1) { std::printf("pagina %d aparece %d vezes\n", page, seen); return 1; }
    }
    for (const auto& tab : kTabs) {
        if (tab.count < 1 || tab.count > 12) return 2;
        for (int i = 0; i < tab.count; ++i) {
            if (!tab.titles[i].label || !*tab.titles[i].label) return 2;
            if (tab.titles[i].page < 0 || tab.titles[i].page >= kMenuPageCount) return 2;   // pagina nova sem kMenuPageCount
        }
    }
    // Estrutura pedida em 26/10 (2a versao): inimigos, drops, pets, camera e minimapa/HUD dentro de
    // Jogabilidade; Graficos e Audio juntos; Cheats fora de Mods; Suporte e Menu juntos.
    const int gameplay = TabOfPage(0), video = TabOfPage(2), cheats = TabOfPage(6), support = TabOfPage(1);
    for (int page : {16, 18, 19, 17, 20, 21, 11, 10, 8}) if (TabOfPage(page) != gameplay) return 3;
    if (cheats == TabOfPage(5) || kTabs[cheats].count != 1) return 4;
    if (TabOfPage(3) != video || video == gameplay) return 5;
    for (int page : {13, 14, 12, 7, 9}) if (TabOfPage(page) != support) return 6;
    if (kTabCount != 6 || gameplay != 0) return 7;
    if (TabOfPage(15) != TabOfPage(4) || kTabs[TabOfPage(5)].count != 1) return 17;
    if (TabOfPage(99) != -1) return 8;

    // Padroes: iguais aos valores iniciais de dev_runtime.h, cada opcao em uma so pagina.
    std::set<const void*> owned;
    int restorable = 0;
    for (int page = 0; page < kMenuPageCount; ++page) {
        const auto defaults = PageDefaults(page);
        if (defaults.Empty()) continue;
        ++restorable;
        if (defaults.Changed() != 0) { std::printf("pagina %d nao bate com o valor inicial\n", page); return 9; }
        for (const auto& [setting, value] : defaults.floats) if (!owned.insert(setting).second) return 10;
        for (const auto& [setting, value] : defaults.bools) if (!owned.insert(setting).second) return 10;
        for (const auto& [setting, value] : defaults.ints) if (!owned.insert(setting).second) return 10;
        // Muda tudo, confere a contagem e restaura.
        for (const auto& [setting, value] : defaults.floats) setting->store(value + .5f);
        for (const auto& [setting, value] : defaults.bools) setting->store(!value);
        for (const auto& [setting, value] : defaults.ints) setting->store(value + 1);
        const int total = int(defaults.floats.size() + defaults.bools.size() + defaults.ints.size());
        if (defaults.Changed() != total) return 11;
        defaults.Apply();
        if (defaults.Changed() != 0) return 12;
    }
    if (restorable < 15) return 13;
    // Restaurar uma pagina nao mexe nas outras.
    camera_mode.store(3);
    damage_taken.store(2);
    PageDefaults(16).Apply();
    if (camera_mode.load() != 3 || damage_taken.load() != 1) return 14;
    PageDefaults(11).Apply();
    if (camera_mode.load() != 0) return 15;
    if (PageDefaults(0).Empty() == false || PageDefaults(1).Empty() == false) return 16;   // Jogo e Relatar bug: nada a restaurar
    std::puts("Menu layout and defaults: pages once, requested tabs, defaults match initial values, page restore isolated passed");
}
