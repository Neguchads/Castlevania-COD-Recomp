#pragma once
#include <atomic>
#include <utility>
#include <vector>
#include "dev_runtime.h"

namespace dev {
// Valores padrao das opcoes de cada pagina do menu dev (os mesmos com que dev_runtime.h inicia).
// "Restaurar padrao" de uma pagina aplica so a lista dela; "Restaurar padroes" aplica todas.
// Idioma do jogo e do menu, mods ligados, perfis e saves ficam de fora: sao escolhas de conteudo.
struct SettingDefaults {
    std::vector<std::pair<std::atomic<float>*, float>> floats;
    std::vector<std::pair<std::atomic<bool>*, bool>> bools;
    std::vector<std::pair<std::atomic<int>*, int>> ints;
    bool Empty() const { return floats.empty() && bools.empty() && ints.empty(); }
    void Apply() const {
        for (const auto& [setting, value] : floats) setting->store(value);
        for (const auto& [setting, value] : bools) setting->store(value);
        for (const auto& [setting, value] : ints) setting->store(value);
    }
    // Quantas opcoes estao fora do padrao.
    int Changed() const {
        int changed = 0;
        for (const auto& [setting, value] : floats) changed += setting->load() != value;
        for (const auto& [setting, value] : bools) changed += setting->load() != value;
        for (const auto& [setting, value] : ints) changed += setting->load() != value;
        return changed;
    }
};
inline SettingDefaults PageDefaults(int page) {
    switch (page) {
        case 16: return {{{&damage_taken, 1}, {&devil_damage_taken, 1}}, {}, {}};
        case 18: return {{{&enemy_hp, 1}, {&enemy_scale, 1}}, {}, {}};
        case 19: return {{{&enemy_random_scale_min, .8f}, {&enemy_random_scale_max, 1.3f}, {&enemy_random_hp_min, 1},
                          {&enemy_random_hp_max, 1.5f}, {&enemy_elite_chance, 0}, {&enemy_elite_scale, 2},
                          {&enemy_elite_hp, 3}},
                         {{&enemy_random, false}}, {}};
        case 20: return {{{&drop_rate, 1}, {&item_scale, 1}}, {{&random_drops, false}}, {}};
        case 21: return {{{&crystal_rate, 1}, {&crystal_amount, 1}}, {}, {}};
        case 2: return {{{&brightness, 0}, {&contrast, 1}, {&saturation, 1}}, {{&show_fps, false}},
                        {{&fps_corner, 1}, {&shader, 0}}};
        case 11: return {{{&camera_distance, 1}, {&camera_height, 0}, {&camera_field, 1}, {&camera_fly_speed, 600},
                          {&camera_yaw, 0}, {&camera_pitch, 0}, {&camera_roll, 0}, {&orbit_smoothing, 8}},
                         {}, {{&camera_mode, 0}}};
        case 10: return {{{&minimap_scale, 1}, {&minimap_opacity, .8f}, {&minimap_x, 1}, {&minimap_y, 0},
                          {&hud_scale, .5f}, {&hud_hide_delay, 1}},
                         {{&minimap_enabled, true}, {&minimap_rotating, true}, {&hud_enabled, true},
                          {&hud_always_visible, false}, {&hud_quick_hide, false}},
                         {}};
        case 3: return {{{&volume, .8f}, {&balance, 0}}, {{&muted, false}, {&mute_unfocused, false}}, {}};
        case 15: return {{}, {{&controller_shortcut, true}, {&invert_stick_x, false}}, {}};
        case 4: return {{{&sensitivity, .012f}},
                        {{&keyboard, true}, {&mouse_camera, true}, {&invert_mouse_x, true}, {&invert_mouse_y, false}}, {}};
        case 6: return {{{&gold_gain, 1}, {&exp_gain, 1}},
                        {{&godmode, false}, {&free_shopping, false}, {&infinite_mp, false}, {&lament_save, false}}, {}};
        case 5: return {{}, {{&texture_dump, false}}, {}};
        case 12: return {{}, {{&perf_log, false}, {&enemy_probe, false}}, {}};
        case 7: return {{}, {{&pause_with_menu, false}}, {}};
        default: return {};
    }
}
}
