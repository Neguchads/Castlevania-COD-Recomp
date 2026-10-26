#include "../src/common/game_cheat_rules.h"
#include <cstdio>
#include <limits>

int main() {
    using namespace dev;
    // Fator: limites e NaN.
    if (ClampDamageFactor(10) != 4 || ClampDamageFactor(0) != .25f || ClampDamageFactor(std::numeric_limits<float>::quiet_NaN()) != 1) return 1;
    // Delta: dano escala, cura e zero ficam iguais, dano nunca vira zero.
    if (ScaleDamageDelta(-10, 2) != -20 || ScaleDamageDelta(-10, .5f) != -5 || ScaleDamageDelta(-10, 1) != -10) return 2;
    if (ScaleDamageDelta(15, 3) != 15 || ScaleDamageDelta(0, 3) != 0) return 3;
    if (ScaleDamageDelta(-1, .25f) != -1 || ScaleDamageDelta(-3, .25f) != -1) return 4;
    if (ScaleDamageDelta(std::numeric_limits<int32_t>::min(), 4) != std::numeric_limits<int32_t>::min()) return 5;
    // Absoluto: reducao que nao mata escala; mais dano pode matar (fica em 0).
    if (ScaleAbsoluteHp(100, 90, 2) != 80 || ScaleAbsoluteHp(100, 90, .5f) != 95 || ScaleAbsoluteHp(100, 40, 2) != 0) return 6;
    // Golpe/roteiro que zera a vida continua zerando; cura e vida ja zerada ficam como o jogo mandou.
    if (ScaleAbsoluteHp(100, 0, .25f) != 0 || ScaleAbsoluteHp(100, -5, .25f) != -5) return 7;
    if (ScaleAbsoluteHp(100, 120, 2) != 120 || ScaleAbsoluteHp(0, 50, 2) != 50 || ScaleAbsoluteHp(100, 100, 2) != 100) return 8;
    if (ScaleAbsoluteHp(100, 99, .25f) != 99) return 9;   // dano minimo 1
    // Godmode continua igual.
    if (!BlockPlayerDamage(true, 6, 0, -5) || BlockPlayerDamage(true, 6, 1, -5) || BlockPlayerDamage(true, 5, 0, -5)) return 10;
    // Fator por ator: slot 0 = Hector, outros slots da raiz = Innocent Devils.
    if (ActorDamageFactor(0, 2, .5f) != 2 || ActorDamageFactor(1, 2, .5f) != .5f || ActorDamageFactor(7, 2, .5f) != .5f) return 11;
    std::puts("Damage rules: factor limits, delta and absolute scaling, lethal and scripted kills, godmode, actor factor passed");
}
