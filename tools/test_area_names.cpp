#include "../src/common/area_names.h"
#include <cstdio>
#include <cstring>
#include <initializer_list>

int main() {
    using namespace dev;
    // Pares da tabela de teleportes do XBE.
    struct Pair { int stage; const char* name; };
    const Pair pairs[]{{2, "Garibaldi Temple"}, {3, "Eneomaos Machine Tower"}, {4, "Aiolon Ruins"}, {6, "Dracula's Castle"},
                       {8, "Forest of Jigramunt"}, {9, "Infinite Corridor"}, {10, "Mortvia Aqueduct"}, {11, "Cordova"},
                       {13, "Baljhet Mountains"}, {5, "Boss Rush"}};
    for (const auto& pair : pairs)
        if (!AreaName(pair.stage) || std::strcmp(AreaName(pair.stage), pair.name)) return 1;
    // Sem confirmacao: nenhum nome inventado; negativos e fora da faixa tambem.
    for (int stage : {0, 1, 7, 12, 14, -1, 127})
        if (AreaName(stage)) return 2;
    std::puts("Area names: teleporter table stages and unknown stages passed");
}
