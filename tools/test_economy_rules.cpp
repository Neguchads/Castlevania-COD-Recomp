#include "../src/common/game_cheat_rules.h"
#include <cstdio>
#include <limits>

int main() {
    using namespace dev;
    // Fator de ganho: 1x a 10x, NaN volta a 1x.
    if (ClampGainFactor(0) != 1 || ClampGainFactor(50) != 10 || ClampGainFactor(std::numeric_limits<float>::quiet_NaN()) != 1) return 1;
    // Ganho escala; gasto e zero ficam iguais; nunca transborda.
    if (ScaleGain(100, 2) != 200 || ScaleGain(7, 1.5f) != 11 || ScaleGain(-50, 3) != -50 || ScaleGain(0, 3) != 0) return 2;
    if (ScaleGain(std::numeric_limits<int32_t>::max(), 10) != std::numeric_limits<int32_t>::max()) return 3;
    EconomyCheats cheats{3, 2, true};
    // Ouro: ganho x3, compra bloqueada.
    if (AdjustEconomyDelta(kStatGold, 0, 100, cheats) != 300 || AdjustEconomyDelta(kStatGold, 0, -500, cheats) != 0) return 4;
    // EXP do Hector: ganho x2; EXP negativo (roteiro) passa intacto.
    if (AdjustEconomyDelta(kStatExp, 0, 40, cheats) != 80 || AdjustEconomyDelta(kStatExp, 0, -40, cheats) != -40) return 5;
    // EXP de Innocent Devil (outro slot) nao multiplica.
    if (AdjustEconomyDelta(kStatExp, 1, 40, cheats) != 40) return 9;
    // Vida e outros atributos nao mudam.
    if (AdjustEconomyDelta(6, 0, -10, cheats) != -10 || AdjustEconomyDelta(6, 0, 10, cheats) != 10 || AdjustEconomyDelta(2, 0, 1, cheats) != 1) return 6;
    // Desligado: compra volta a gastar.
    if (AdjustEconomyDelta(kStatGold, 0, -500, EconomyCheats{}) != -500 || AdjustEconomyDelta(kStatGold, 0, 100, EconomyCheats{}) != 100) return 7;
    // Vida cheia: so vivo, maximo valido, nunca reduz.
    if (RefillHp(10, 300) != 300 || RefillHp(0, 300) != 0 || RefillHp(10, 0) != 10 || RefillHp(10, 5000) != 10 || RefillHp(400, 300) != 400) return 8;
    // Save de Lament of Innocence: so a flag 0x4002 vira 1, e so com a opcao ligada.
    if (LamentSaveFlag(0x4002, 0, true) != 1 || LamentSaveFlag(0x4002, 0, false) != 0 || LamentSaveFlag(0x4002, 1, false) != 1) return 10;
    if (LamentSaveFlag(0x4001, 0, true) != 0 || LamentSaveFlag(0x30008006, 0, true) != 0 || LamentSaveFlag(0x4002, 1, true) != 1) return 11;
    std::puts("Economy rules: gain factor limits, gold and EXP scaling, devil EXP untouched, free shopping, other stats untouched, HP refill, Lament save flag passed");
}
