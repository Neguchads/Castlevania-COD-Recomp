#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include "enemy_editor_rules.h"

namespace dev {
// Mod "Editor": mods/Editor/enemies.toml guarda os ajustes por tipo de inimigo; valem com o mod ligado.
inline constexpr const char* kEditorMod = "Editor";
struct EnemyRow { uint32_t type = 0; int seen = 0; EnemyEdit edit; };
// Chamados pelos hooks do jogo (rapidos, sem trava no caminho comum).
float EnemyHpEdit(uint32_t type);
float EnemyScaleEdit(uint32_t type);
void NoteEnemy(uint32_t type, const std::string& name);
// Estado do ator: passo de escala (fator * sorteio) e sorteio guardado ate o ator mudar de tipo ou nascer de novo.
struct ActorEdit { ScaleStep step; EnemyRoll roll; bool rolled = false; };
ActorEdit EnemyActorEdit(uint32_t actor, uint32_t type, float current_scale, float factor, const EnemyRandomSettings& random);
// Menu e mods.
bool EnemyEditorActive();
void RefreshEnemyEditor(const std::filesystem::path& mod_root, bool active);
std::vector<EnemyRow> EnemyEditorRows();
void SetEnemyEdit(uint32_t type, float hp, float scale);
bool SaveEnemyEdits();
}
