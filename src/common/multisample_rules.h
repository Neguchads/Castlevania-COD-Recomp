#pragma once
#include <atomic>
#include <cstdint>

// D3DRS_MULTISAMPLEMODE (back buffer) e D3DRS_MULTISAMPLERENDERTARGETMODE (outros alvos), como o XDK faz em
// sub_003B9EC0: numa superficie linear, 2X liga CENTER_CORNER_2 e 4X liga SQUARE_OFFSET_4. A grade logica
// encolhe para (w + 1) / 2 e o hardware expande cada pixel logico em 2x1 ou 2x2 pixels da memoria.
// O COD usa 4X so no volume de sombra (sub_00043C00), transformado para 320x240 sobre o alvo 640x480.
namespace multisample_rules {
struct Factor {
    uint32_t x = 1, y = 1;
};
inline Factor ModeFactor(uint32_t mode, bool swizzled) {
    if (!mode || swizzled) return {};
    return mode == 2 ? Factor{2, 2} : Factor{2, 1};
}
inline uint32_t Logical(uint32_t size, uint32_t factor) { return factor > 1 ? (size + 1) / factor : size; }
inline uint32_t EffectiveMode(bool back_buffer, uint32_t mode, uint32_t render_target_mode) {
    return back_buffer ? mode : render_target_mode;
}
// cabecalho guest do back buffer: o runtime D3D8 grava, o renderer compara com o alvo do desenho
inline std::atomic<uint32_t> back_buffer{0};
}  // namespace multisample_rules
