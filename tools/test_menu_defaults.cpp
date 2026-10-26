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
        for (int i = 0; i < tab.count; ++i) if (!tab.titles[i].label || !*tab.titles[i].label) return 2;
    }
    // Estrutura pedida: camera e minimapa/HUD em Graficos; Cheats fora de Mods; inimigos juntos;
    // Drops e itens e Pets em abas proprias.
    const int graphics = TabOfPage(2), enemies = TabOfPage(18), cheats = TabOfPage(6);
    if (TabOfPage(11) != graphics || TabOfPage(10) != graphics) return 3;
    if (cheats == TabOfPage(5) || kTabs[cheats].count != 1) return 4;
    if (TabOfPage(17) != enemies || TabOfPage(19) != enemies || kTabs[enemies].count != 3) return 5;
    if (TabOfPage(20) == TabOfPage(21) || kTabs[TabOfPage(20)].count != 1 || kTabs[TabOfPage(21)].count != 1) return 6;
    if (TabOfPage(0) != TabOfPage(16) || TabOfPage(0) == enemies) return 7;
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
