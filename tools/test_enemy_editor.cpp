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
    std::puts("Enemy editor: scale per actor without stacking, restore, game change, limits and neutral edit passed");
}
