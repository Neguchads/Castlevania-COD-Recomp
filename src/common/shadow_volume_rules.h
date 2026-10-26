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
}
