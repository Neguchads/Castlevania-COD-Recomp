#include "../src/common/enemy_editor_rules.h"
#include <cmath>
#include <cstdio>

int main() {
    using namespace dev;
    // Primeira vez: escala do jogo (1) vira base e recebe o fator.
    auto step = NextScale(1.0f, nullptr, 2.0f);
    if (!step.write || step.value != 2.0f || step.state.base != 1.0f || step.state.applied != 2.0f) return 1;
    // Chamada seguinte com a escala ja aplicada: nada a escrever (nao acumula 2x2x2...).
    const ScaleState applied = step.state;
    step = NextScale(2.0f, &applied, 2.0f);
    if (step.write || step.state.base != 1.0f) return 2;
    // Fator volta a 1x: restaura a escala original do jogo.
    step = NextScale(2.0f, &applied, 1.0f);
    if (!step.write || step.value != 1.0f) return 3;
    // O jogo trocou a escala (ou o ator foi reaproveitado): a nova escala vira base.
    step = NextScale(1.5f, &applied, 2.0f);
    if (!step.write || step.value != 3.0f || step.state.base != 1.5f) return 4;
    // Sem edicao e sem historico: nao escreve nada.
    step = NextScale(1.0f, nullptr, 1.0f);
    if (step.write) return 5;
    // Escala invalida do jogo nunca e tocada.
    if (NextScale(0.0f, nullptr, 2.0f).write || NextScale(NAN, nullptr, 2.0f).write || NextScale(-1.0f, nullptr, 2.0f).write) return 6;
    // Limites: tamanho 0,5x a 3x; NaN volta a 1x.
    if (ClampEnemyScale(9) != kMaxEnemyScale || ClampEnemyScale(.1f) != kMinEnemyScale || ClampEnemyScale(NAN) != 1.0f) return 7;
    step = NextScale(1.0f, nullptr, 10.0f);
    if (step.value != kMaxEnemyScale) return 8;
    // Tipo de inimigo editavel so abaixo do limite da tabela.
    if (!EditableEnemyType(0) || !EditableEnemyType(kEnemyTypeLimit - 1) || EditableEnemyType(kEnemyTypeLimit)) return 9;
    // Edicao padrao nao muda nada.
    const EnemyEdit neutral;
    if (!neutral.Neutral() || EnemyEdit{2, 1, {}}.Neutral() || EnemyEdit{1, 1.5f, {}}.Neutral()) return 10;
    // Barra global de Dificuldade vezes o ajuste do tipo, limitado a 0,5x-3x no total.
    if (CombinedEnemyScale(1, 1) != 1 || CombinedEnemyScale(2, 1.5f) != 3 || CombinedEnemyScale(2, .5f) != 1) return 11;
    if (CombinedEnemyScale(3, 3) != kMaxEnemyScale || CombinedEnemyScale(.5f, .5f) != kMinEnemyScale || CombinedEnemyScale(NAN, 2) != 2) return 12;
    // Sorteio: u em [0,1) vira um valor da faixa; faixa invertida e corrigida; limites e NaN.
    if (RollInRange(1, 2, 0, kMinEnemyScale, kMaxEnemyScale) != 1 || RollInRange(1, 2, .5f, kMinEnemyScale, kMaxEnemyScale) != 1.5f) return 13;
    if (RollInRange(2, 1, .5f, kMinEnemyScale, kMaxEnemyScale) != 1.5f || RollInRange(.1f, 9, 1, kMinEnemyScale, kMaxEnemyScale) != kMaxEnemyScale) return 14;
    if (RollInRange(NAN, 2, .5f, kMinEnemyScale, kMaxEnemyScale) != 1.5f || RollInRange(1, 2, NAN, kMinEnemyScale, kMaxEnemyScale) != 1) return 15;
    // Desligado: nada muda, mesmo com elite garantido.
    EnemyRandomSettings random;
    random.scale_min = 2; random.scale_max = 2; random.elite_chance = 1;
    const auto off = RollEnemy(random, .5f, .5f, 0);
    if (off.scale != 1 || off.hp != 1 || off.elite) return 16;
    // Ligado: faixa de tamanho e de vida; elite multiplica por cima quando o sorteio cai abaixo da chance.
    random.enabled = true; random.scale_min = 1; random.scale_max = 2; random.hp_min = 1; random.hp_max = 3;
    random.elite_chance = .25f; random.elite_scale = 2; random.elite_hp = 2;
    auto roll = RollEnemy(random, .5f, .5f, .9f);
    if (roll.scale != 1.5f || roll.hp != 2 || roll.elite) return 17;
    roll = RollEnemy(random, .5f, .5f, .1f);
    if (!roll.elite || roll.scale != 3 || roll.hp != 4) return 18;
    // Chance 0 nunca vira elite; chance invalida conta como 0.
    random.elite_chance = 0;
    if (RollEnemy(random, .5f, .5f, 0).elite) return 19;
    random.elite_chance = NAN;
    if (RollEnemy(random, .5f, .5f, 0).elite) return 20;
    // Vida cheia acompanha o sorteio; ferido e vida invalida ficam.
    if (RolledCurrentHp(185, 185, 370) != 370 || RolledCurrentHp(185, 185, 93) != 93) return 21;
    if (RolledCurrentHp(120, 185, 370) != 120) return 22;
    if (RolledCurrentHp(0, 0, 0) != 0 || !std::isnan(RolledCurrentHp(NAN, 185, 370))) return 23;
    std::puts("Enemy editor: scale per actor without stacking, restore, game change, limits and neutral edit passed");
}
