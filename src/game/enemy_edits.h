#pragma once
#include <cstdint>
#include "xdk/hle.h"

namespace dev {
// Jogabilidade > inimigos: registra tipo/nome e aplica tamanho (todos x tipo x sorteio) e vida sorteada no ator (so memoria).
// hp_return: so no retorno de sub_00060010, onde o topo do x87 e a vida maxima do ator.
void ApplyEnemyEdits(X86Context& ctx, uint8_t* base, uint32_t actor, bool hp_return = false);
bool VerifyEnemyEditorHooks();
// Multiplica a escala nativa de um objeto de cena (sub_0035BA40); falso se o objeto ou a escala nao servem.
bool ScaleSceneObject(X86Context& ctx, uint8_t* base, uint32_t object, float factor);
// Item no chao recem-criado: o tamanho dele volta por cima se o jogo o reescalar.
void NoteItemObject(const uint8_t* base, uint32_t object);
// Chamado pelo hook de sub_0035BA40 (indicators.cpp) depois do original: reaplica o fator escolhido.
// Nossas gravacoes chamam o original direto, sem passar pelo hook.
void GameScaleSet(X86Context& ctx, uint8_t* base, uint32_t object);
}
