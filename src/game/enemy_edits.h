#pragma once
#include <cstdint>
#include "xdk/hle.h"

namespace dev {
// Aba Inimigos: registra tipo/nome e aplica tamanho (todos x tipo x sorteio) e vida sorteada no ator (so memoria).
// hp_return: so no retorno de sub_00060010, onde o topo do x87 e a vida maxima do ator.
void ApplyEnemyEdits(X86Context& ctx, uint8_t* base, uint32_t actor, bool hp_return = false);
bool VerifyEnemyEditorHooks();
}
