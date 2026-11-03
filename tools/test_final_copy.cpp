#include "../src/common/final_copy_rules.h"
#include <cstdio>
#include <limits>
struct Vertex {float x,y,z,w,u,v;};
int main() {
    const std::array<Vertex,4> original{{{0,0,0,1,0,-16},{640,0,0,1,640,-16},
        {640,480,0,1,640,496},{0,480,0,1,0,496}}};
    auto copy=original;
    auto repair=[&](auto& data) {
        return cod_graphics::RepairFinalCopy({reinterpret_cast<const uint8_t*>(original.data()),sizeof(original)},
            sizeof(Vertex),0,{reinterpret_cast<uint8_t*>(data.data()),sizeof(data)},sizeof(Vertex),16);
    };
    if(!repair(copy))return 1;
    for(size_t i=0;i<4;++i)if(copy[i].v!=original[i].y || copy[i].u!=original[i].u ||
        copy[i].x!=original[i].x || copy[i].z!=original[i].z || copy[i].w!=original[i].w)return 2;
    if(original[0].v!=-16 || original[2].v!=496)return 3;
    copy=original;copy[1].v=0;
    const auto invalid=copy;
    if(repair(copy) || std::memcmp(copy.data(),invalid.data(),sizeof(copy)))return 4;
    copy=original;copy[0].u=std::numeric_limits<float>::quiet_NaN();
    if(repair(copy))return 5;
    if(!cod_graphics::FinalCopy(0xE68AA3D333B6F207ull,0xB43F89051B292965ull,true,640,480,false,false,4) ||
       cod_graphics::FinalCopy(0,0xB43F89051B292965ull,true,640,480,false,false,4) ||
       cod_graphics::FinalCopy(0xE68AA3D333B6F207ull,0xB43F89051B292965ull,false,640,480,false,false,4) ||
       cod_graphics::FinalCopy(0xE68AA3D333B6F207ull,0xB43F89051B292965ull,true,512,512,false,false,4))return 6;
    std::array<uint8_t,4> small{};
    if(cod_graphics::RepairFinalCopy(small,24,0,small,24,16))return 7;
    std::puts("Final copy: captured quad, UV repair, source/depth preservation and negative guards passed");
}
