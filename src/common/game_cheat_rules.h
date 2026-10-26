#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace dev {
inline bool BlockPlayerDamage(bool enabled, uint32_t stat, uint32_t slot, int32_t delta) {
    return enabled && stat == 6 && slot == 0 && delta < 0;
}
// Dificuldade: fator do dano em vida, de 0,25x a 4x (NaN volta a 1x).
inline constexpr float kMinDamageFactor = .25f, kMaxDamageFactor = 4.0f;
inline float ClampDamageFactor(float factor) {
    return std::isfinite(factor) ? std::clamp(factor, kMinDamageFactor, kMaxDamageFactor) : 1.0f;
}
// A raiz de atributos B2A350 so guarda o grupo: slot 0 = Hector, demais = Innocent Devils (logs de 25/10:
// em 36 golpes so ID 0/slot 0 e ID 1/slot 1). A vida dos inimigos fica fora dela.
inline float ActorDamageFactor(uint32_t slot, float player, float devil) { return slot == 0 ? player : devil; }
// Delta de vida (sub_00336070): so dano (negativo) muda; dano nunca vira zero nem cura.
inline int32_t ScaleDamageDelta(int32_t delta, float factor) {
    if (delta >= 0) return delta;
    const double scaled = std::round(double(delta) * ClampDamageFactor(factor));
    return int32_t(std::clamp(scaled, double(INT32_MIN), -1.0));
}
// Vida absoluta (sub_00337A00): so reducao que nao mata muda; golpe ou script que zera a vida
// continua zerando (senao morte por roteiro deixaria o inimigo vivo).
inline int32_t ScaleAbsoluteHp(int32_t hp, int32_t next, float factor) {
    if (hp <= 0 || next <= 0 || next >= hp) return next;
    const double damage = std::max(1.0, std::round(double(hp - next) * ClampDamageFactor(factor)));
    return int32_t(std::max(0.0, double(hp) - damage));
}

// Economia: atributo 3 = EXP, 111 = ouro (raiz+0xAEB4, o jogo limita a 9.999.999).
inline constexpr uint32_t kStatExp = 3, kStatGold = 111;
inline constexpr int32_t kMaxGold = 9999999;
inline constexpr float kMinGainFactor = 1.0f, kMaxGainFactor = 10.0f;
struct EconomyCheats { float gold_gain = 1, exp_gain = 1; bool free_shopping = false; };
inline float ClampGainFactor(float factor) {
    return std::isfinite(factor) ? std::clamp(factor, kMinGainFactor, kMaxGainFactor) : 1.0f;
}
// So ganho (positivo) muda; nunca transborda o int32 do jogo.
inline int32_t ScaleGain(int32_t delta, float factor) {
    if (delta <= 0) return delta;
    const double scaled = std::round(double(delta) * ClampGainFactor(factor));
    return int32_t(std::min(scaled, double(INT32_MAX)));
}
// Delta de atributo (sub_00336070): ouro (da raiz, qualquer ator) e EXP so do Hector (slot 0); os Innocent
// Devils e os demais atributos passam intactos. Com compras gratis, qualquer gasto de ouro e zerado.
inline int32_t AdjustEconomyDelta(uint32_t stat, uint32_t slot, int32_t delta, const EconomyCheats& cheats) {
    if (stat == kStatGold) return delta < 0 && cheats.free_shopping ? 0 : ScaleGain(delta, cheats.gold_gain);
    if (stat == kStatExp && slot == 0) return ScaleGain(delta, cheats.exp_gain);
    return delta;
}
// Vida cheia: so com o jogador vivo (nao desfaz morte) e vida maxima valida (limite 999 do jogo).
inline int32_t RefillHp(int32_t hp, int32_t max_hp) {
    return hp > 0 && max_hp > 0 && max_hp <= 999 && hp < max_hp ? max_hp : hp;
}

// Bonus do save de Lament of Innocence: a tela de save liga a flag 0x4002 ao achar o save de PS2 (0x33F310);
// jogo novo e carregamento (0x2F4116, 0x3218A3) leem 0x4002 e gravam a flag 0x30008006 (Moai) no save.
// No Xbox o save de PS2 nunca existe; simular so responde "achou" para essa flag.
inline constexpr uint32_t kFlagLamentSave = 0x4002;
inline uint32_t LamentSaveFlag(uint32_t flag, uint32_t value, bool simulate) {
    return simulate && flag == kFlagLamentSave && value == 0 ? 1 : value;
}
}
