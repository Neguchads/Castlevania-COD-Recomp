#pragma once
#include <cstdint>
#include "xdk/hle.h"

namespace dev {
// Editor de inimigos: registra tipo/nome e aplica o tamanho do tipo no ator (so memoria).
void ApplyEnemyEdits(X86Context& ctx, uint8_t* base, uint32_t actor);
bool VerifyEnemyEditorHooks();
}
