#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>

namespace dev {
// Editor de inimigos (mod "Editor"): ajustes por tipo, so na memoria; o jogo original nao muda.
// Tipo = word [ator+2] (sub_000276B0 devolve o proprio ator; [ator+0] == 2 e inimigo).
inline constexpr uint32_t kEnemyTypeLimit = 1024;
inline constexpr bool EditableEnemyType(uint32_t type) { return type < kEnemyTypeLimit; }
inline constexpr float kMinEnemyScale = .5f, kMaxEnemyScale = 3.0f;
inline float ClampEnemyScale(float factor) {
    return std::isfinite(factor) ? std::clamp(factor, kMinEnemyScale, kMaxEnemyScale) : 1.0f;
}
// Barra global de Dificuldade vezes o ajuste do tipo no mod Editor.
inline float CombinedEnemyScale(float global, float edit) {
    return ClampEnemyScale(ClampEnemyScale(global) * ClampEnemyScale(edit));
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
