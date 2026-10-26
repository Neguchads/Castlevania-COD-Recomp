#pragma once
#include <cstdint>
namespace cod_hud {
inline bool PetTextureSet(uint32_t noise_width,uint32_t noise_height,
                          uint32_t mask_width,uint32_t mask_height,uint32_t mask_data,
                          uint32_t colour_width,uint32_t colour_height,uint32_t colour_data) {
    // O titulo compartilha shaders, mas usa ruido256 e uma terceira textura512.
    return noise_width==64 && noise_height==64 && mask_width==256 && mask_height==256 &&
        colour_width==256 && colour_height==256 && mask_data!=0 && mask_data==colour_data;
}
inline bool KeepVisible(bool enabled,bool always,bool native_menu) {
    return enabled && always && !native_menu;
}
inline void RefreshVisibility(float& timer,uint8_t& alpha,uint32_t& state) {
    timer=5.0f;alpha=0x80;
    // Modo automatico visivel, com o prazo nativo renovado a cada update.
    state=(state & ~0x1Eu) | 8u;
}
// Par de shaders observado nas capturas 220612 e 220802 do seletor animado.
template<class MenuQuery>
inline bool PetHighlight(uint64_t vertex,uint64_t pixel,MenuQuery native_menu) {
    return vertex==0x6B7F98AECCA424BDull && pixel==0xB88574E78D5DD008ull && !native_menu();
}
inline void ScaleHighlight(float* multiplier,float* offset,float scale,float height) {
    multiplier[0]*=scale;multiplier[1]*=scale;
    offset[0]*=scale;offset[1]=height+(offset[1]-height)*scale;
}
}
