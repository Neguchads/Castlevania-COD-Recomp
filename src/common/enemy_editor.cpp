#include "enemy_editor.h"
#include <array>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <unordered_map>
#include <toml++/toml.hpp>
#include "core/log.h"
#include "dev_runtime.h"
#include "game_cheat_rules.h"

namespace dev {
namespace fs = std::filesystem;
namespace {
std::array<std::atomic<float>, kEnemyTypeLimit> hp_edits, scale_edits;
std::array<std::atomic<int>, kEnemyTypeLimit> seen;
std::array<std::atomic<bool>, kEnemyTypeLimit> named;
std::atomic<bool> active{false};
std::mutex lock;
std::map<uint32_t, std::string> names;   // tipo -> nome do painel
std::unordered_map<uint32_t, ScaleState> actors;
fs::path root;
struct Init { Init() { for (auto& v : hp_edits) v = 1; for (auto& v : scale_edits) v = 1; } } init;
fs::path File() { return root.empty() ? Root()/"mods"/kEditorMod/"enemies.toml" : root/"enemies.toml"; }
}
float EnemyHpEdit(uint32_t type) {
    return active.load(std::memory_order_relaxed) && EditableEnemyType(type) ? hp_edits[type].load(std::memory_order_relaxed) : 1.0f;
}
float EnemyScaleEdit(uint32_t type) {
    return active.load(std::memory_order_relaxed) && EditableEnemyType(type) ? scale_edits[type].load(std::memory_order_relaxed) : 1.0f;
}
void NoteEnemy(uint32_t type, const std::string& name) {
    if (!EditableEnemyType(type)) return;
    if (seen[type].fetch_add(1, std::memory_order_relaxed) == 0)
        LOGR("editor: enemy type %u seen name='%s'", type, name.c_str());
    // So trava ate o tipo ganhar nome.
    if (name.empty() || named[type].load(std::memory_order_relaxed)) return;
    std::lock_guard guard(lock);
    names[type] = name;
    named[type] = true;
}
ScaleStep EnemyScaleStep(uint32_t actor, float current, float factor) {
    std::lock_guard guard(lock);
    // Atores mudam de endereco a cada sala; limpar de vez em quando so faz a escala atual virar base de novo.
    if (actors.size() > 4096) actors.clear();
    const auto it = actors.find(actor);
    const auto step = NextScale(current, it == actors.end() ? nullptr : &it->second, factor);
    if (step.state.base > 0 && std::isfinite(step.state.base)) actors[actor] = step.state;
    return step;
}
bool EnemyEditorActive() { return active.load(); }
void RefreshEnemyEditor(const fs::path& mod_root, bool enabled) {
    std::lock_guard guard(lock);
    root = mod_root;
    for (auto& v : hp_edits) v = 1;
    for (auto& v : scale_edits) v = 1;
    std::error_code ec;
    if (!mod_root.empty() && fs::is_regular_file(File(), ec)) {
        try {
            const auto table = toml::parse_file(File().string());
            if (const auto* enemies = table["enemy"].as_table()) {
                for (const auto& [key, node] : *enemies) {
                    const auto* entry = node.as_table();
                    char* end = nullptr;
                    const unsigned long type = std::strtoul(std::string(key.str()).c_str(), &end, 10);
                    if (!entry || !end || *end || !EditableEnemyType(uint32_t(type))) continue;
                    hp_edits[type] = ClampEnemyHpFactor((*entry)["hp"].value_or(1.0f));
                    scale_edits[type] = ClampEnemyScale((*entry)["scale"].value_or(1.0f));
                    if (auto name = (*entry)["name"].value<std::string>(); name && names[uint32_t(type)].empty())
                        names[uint32_t(type)] = *name;
                }
            }
        } catch (const toml::parse_error& error) {
            LOGR("editor: %s ignored: %s", File().string().c_str(), std::string(error.description()).c_str());
        }
    }
    active = enabled && !mod_root.empty();
    LOGR("editor: enemy edits %s", active.load() ? "active" : "off");
}
std::vector<EnemyRow> EnemyEditorRows() {
    std::vector<EnemyRow> rows;
    std::lock_guard guard(lock);
    for (uint32_t type = 0; type < kEnemyTypeLimit; ++type) {
        const float hp = hp_edits[type], scale = scale_edits[type];
        const int count = seen[type];
        const auto name = names.find(type);
        if (!count && hp == 1 && scale == 1 && name == names.end()) continue;
        rows.push_back({type, count, {hp, scale, name == names.end() ? std::string{} : name->second}});
    }
    return rows;
}
void SetEnemyEdit(uint32_t type, float hp, float scale) {
    if (!EditableEnemyType(type)) return;
    hp_edits[type] = ClampEnemyHpFactor(hp);
    scale_edits[type] = ClampEnemyScale(scale);
}
bool SaveEnemyEdits() {
    toml::table enemies;
    for (const auto& row : EnemyEditorRows()) {
        if (row.edit.Neutral()) continue;
        toml::table entry;
        if (!row.edit.name.empty()) entry.insert_or_assign("name", row.edit.name);
        entry.insert_or_assign("hp", double(row.edit.hp));
        entry.insert_or_assign("scale", double(row.edit.scale));
        enemies.insert_or_assign(std::to_string(row.type), std::move(entry));
    }
    toml::table file;
    file.insert_or_assign("enemy", std::move(enemies));
    fs::path path;
    { std::lock_guard guard(lock); path = File(); }
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << "# Editor de inimigos do COD recomp: hp e scale por tipo (word [ator+2]).\n" << file << "\n";
    const bool ok = bool(out);
    LOGR("editor: enemy edits saved to %s ok=%d", path.string().c_str(), int(ok));
    return ok;
}
}
