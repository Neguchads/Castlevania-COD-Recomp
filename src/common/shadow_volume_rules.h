#pragma once
#include <cstdint>
namespace cod_graphics {
inline bool ShadowVolume(bool stencil,uint32_t colour_mask,bool depth_write,bool depth_test,bool triangles) {
    return stencil && colour_mask==0 && !depth_write && depth_test && triangles;
}
inline bool DepthClip(bool stencil,uint32_t colour_mask,bool depth_write,bool depth_test,bool triangles) {
    // Volumes extrudidos precisam manter a contagem stencil alem do plano distante.
    return !ShadowVolume(stencil,colour_mask,depth_write,depth_test,triangles);
}
// operacoes stencil que contam faces (GL: INCR 1E02, DECR 1E03, INCR_WRAP 8507, DECR_WRAP 8508)
inline bool CountingOp(uint32_t op) {
    return op==0x1E02 || op==0x1E03 || op==0x8507 || op==0x8508;
}
// Sem o volume o stencil fica zerado, entao o composite da sombra nao escurece nada.
// Nas 16 capturas F3 so o volume do personagem (VS B0101FC1) casa: func ALWAYS (207), pass INCR_WRAP/DECR.
// Exige a contagem (z-pass ou z-fail) para nao engolir mascaras stencil com REPLACE/ZERO.
inline bool KeepDraw(bool character_shadow,bool stencil,uint32_t colour_mask,bool depth_write,bool depth_test,bool triangles,
                     uint32_t func,uint32_t pass,uint32_t zfail) {
    return character_shadow || !ShadowVolume(stencil,colour_mask,depth_write,depth_test,triangles) ||
           func!=0x207 || !(CountingOp(pass) || CountingOp(zfail));
}
}
