#pragma once
#include <cstdint>

namespace dev {
// Nome da area pelo ID de mapa ([0xB234F4]). Fonte: tabela dos teleportes no XBE (0x4E91F0, entradas de
// 6 bytes: mapa, sala, flags, ?, ID de texto) cruzada com strdataue.dar (IDs 0x4E a 0x58). Mapas sem
// teleporte (0, 1, 7, 12 e acima de 13) ainda nao tem nome confirmado.
inline const char* AreaName(int stage) {
    switch (stage) {
    case 2: return "Garibaldi Temple";
    case 3: return "Eneomaos Machine Tower";
    case 4: return "Aiolon Ruins";
    case 5: return "Boss Rush";
    case 6: return "Dracula's Castle";
    case 8: return "Forest of Jigramunt";
    case 9: return "Infinite Corridor";
    case 10: return "Mortvia Aqueduct";
    case 11: return "Cordova";
    case 13: return "Baljhet Mountains";
    default: return nullptr;
    }
}
}
