#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include "game_cheat_rules.h"

namespace dev {
// Editor de inimigos (mod "Editor"): ajustes por tipo, so na memoria; o jogo original nao muda.
// Tipo = word [ator+2] (sub_000276B0 devolve o proprio ator; [ator+0] == 2 e inimigo).
inline constexpr uint32_t kEnemyTypeLimit = 1024;
inline constexpr bool EditableEnemyType(uint32_t type) { return type < kEnemyTypeLimit; }
inline constexpr float kMinEnemyScale = .5f, kMaxEnemyScale = 3.0f;
inline float ClampEnemyScale(float factor) {
    return std::isfinite(factor) ? std::clamp(factor, kMinEnemyScale, kMaxEnemyScale) : 1.0f;
}
// Barra global de Todos os inimigos vezes o ajuste do tipo no mod Editor.
inline float CombinedEnemyScale(float global, float edit) {
    return ClampEnemyScale(ClampEnemyScale(global) * ClampEnemyScale(edit));
}
// Sorteio por inimigo: u em [0,1) vira um ponto da faixa [lo,hi] (invertida e corrigida, NaN vira 1x).
inline float RollInRange(float lo, float hi, float u, float limit_lo, float limit_hi) {
    auto sane = [&](float v) { return std::clamp(std::isfinite(v) ? v : 1.0f, limit_lo, limit_hi); };
    lo = sane(lo); hi = sane(hi);
    if (lo > hi) std::swap(lo, hi);
    u = std::isfinite(u) ? std::clamp(u, 0.0f, 1.0f) : 0.0f;
    return lo + (hi - lo) * u;
}
struct EnemyRandomSettings {
    bool enabled = false;
    float scale_min = 1, scale_max = 1, hp_min = 1, hp_max = 1;
    float elite_chance = 0, elite_scale = 2, elite_hp = 3;   // chance de 0 a 1
};
struct EnemyRoll { float scale = 1, hp = 1; bool elite = false; };
// Os limites finais (tamanho 0,5x-3x, vida 0,5x-5x) valem na aplicacao, depois de somar global e tipo.
inline EnemyRoll RollEnemy(const EnemyRandomSettings& random, float u_scale, float u_hp, float u_elite) {
    if (!random.enabled) return {};
    EnemyRoll roll{RollInRange(random.scale_min, random.scale_max, u_scale, kMinEnemyScale, kMaxEnemyScale),
                   RollInRange(random.hp_min, random.hp_max, u_hp, kMinEnemyHpFactor, kMaxEnemyHpFactor), false};
    const float chance = std::isfinite(random.elite_chance) ? std::clamp(random.elite_chance, 0.0f, 1.0f) : 0.0f;
    if (chance > 0 && u_elite < chance) {
        roll.elite = true;
        roll.scale *= ClampEnemyScale(random.elite_scale);
        roll.hp *= ClampEnemyHpFactor(random.elite_hp);
    }
    return roll;
}
// Vida sorteada por ator: o jogo nasce o inimigo com a vida do tipo (raw_max). Vida cheia nesse valor sobe
// (ou desce) para a maxima sorteada; ferido fica como esta, e o jogo corta o que passar da maxima.
inline float RolledCurrentHp(float current, double raw_max, double rolled_max) {
    if (!(raw_max > 0) || !(rolled_max > 0) || !std::isfinite(current)) return current;
    return current == float(raw_max) ? float(rolled_max) : current;
}
struct EnemyEdit {
    float hp = 1, scale = 1;
    std::string name;
    bool Neutral() const { return hp == 1 && scale == 1; }
};
// Escala por ator, em [ator+0xA0] (no da cena em +0x80, vetor em +0x20). base = escala que o jogo deu;
// applied = base * fator. Se a atual nao bate com a aplicada, o jogo trocou a escala ou reaproveitou o ator,
// e a atual vira a nova base: o fator nunca acumula.
struct ScaleState { float base = 1, applied = 1; };
struct ScaleStep { bool write = false; float value = 1; ScaleState state; };
inline ScaleStep NextScale(float current, const ScaleState* known, float factor) {
    if (!std::isfinite(current) || current <= 0) return {false, current, {current, current}};
    const float base = known && current == known->applied ? known->base : current;
    const float target = base * ClampEnemyScale(factor);
    return {target != current, target, {base, target}};
}
}
