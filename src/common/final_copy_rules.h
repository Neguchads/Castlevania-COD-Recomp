#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <span>
namespace cod_graphics {
inline bool FinalCopy(uint64_t vertex,uint64_t pixel,bool render_target,uint32_t width,uint32_t height,
                      bool blend,bool depth,uint32_t count) {
    return vertex==0xE68AA3D333B6F207ull && pixel==0xB43F89051B292965ull &&
        render_target && width==640 && height==480 && !blend && !depth && count==4;
}
// Corrige somente o quad0..640/480 com UV0..640/-16..496 visto nas capturas.
// Opera na copia de upload; nunca escreve nos vertices guest ou na imagem fonte.
inline bool RepairFinalCopy(std::span<const uint8_t> positions,uint32_t position_stride,uint32_t position_offset,
                            std::span<uint8_t> texture,uint32_t texture_stride,uint32_t texture_offset) {
    if(!position_stride || !texture_stride ||
       uint64_t(position_offset)+3ull*position_stride+8>positions.size() ||
       uint64_t(texture_offset)+3ull*texture_stride+8>texture.size())return false;
    std::array<std::array<float,2>,4> xy{},uv{};
    unsigned corners=0;
    for(size_t i=0;i<4;++i) {
        std::memcpy(xy[i].data(),positions.data()+position_offset+i*position_stride,8);
        std::memcpy(uv[i].data(),texture.data()+texture_offset+i*texture_stride,8);
        const auto x=xy[i][0],y=xy[i][1],u=uv[i][0],v=uv[i][1];
        if(!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(u) || !std::isfinite(v) ||
           (x!=0 && x!=640) || (y!=0 && y!=480) || std::abs(u-x)>.001f ||
           std::abs(v-(y==0 ? -16.f : 496.f))>.001f)return false;
        corners|=1u<<((x==640 ? 1u:0u)+(y==480 ? 2u:0u));
    }
    if(corners!=15)return false;
    for(size_t i=0;i<4;++i)
        std::memcpy(texture.data()+texture_offset+i*texture_stride+4,&xy[i][1],4);
    return true;
}
}
