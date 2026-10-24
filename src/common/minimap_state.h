#pragma once
#include <cstdint>
namespace cod_map {
struct RoomState { bool explored=false;uint32_t type=0; };
// Tipo da sala em 2FD450 e exploracao dos fragmentos em 2F67D0.
template<class Reader> RoomState NativeRoomState(int stage,int room,uint32_t fragments,int count,Reader read) {
    RoomState result;
    if(stage<0 || stage>31 || room<0 || room>255)return result;
    uint32_t database=0,stages=0,records=0,flags=0;
    if(!read(0xB24640,database) || !database || !read(database+12,stages) || !stages ||
       !read(stages+uint32_t(stage)*12+8,records) || !records)return result;
    const uint32_t record=records+uint32_t(room)*20;
    if(!read(record,result.type))return {};
    if(count<1 || count>127 || !fragments || !read(0x548CC8,flags) || !flags)return result;
    for(int i=0;i<count;++i) {
        uint32_t index=0;uint8_t visited=0;
        if(read(fragments+uint32_t(i)*16,index) && read(flags+(index&0x3FFF),visited) && (visited&1)) {
            result.explored=true;break;
        }
    }
    return result;
}
struct RoomTint {int r,g,b;float fill,edge;};
inline RoomTint Tint(RoomState state,bool active) {
    // Áreas futuras continuam visíveis, mas ficam mais escuras que as salas exploradas.
    if(!state.explored && !active)return {58,79,115,.42f,.70f};
    if(state.type==0x490)return {175,105,133,active?.75f:.58f,.9f};
    if(state.type==0x491)return {90,151,137,active?.75f:.58f,.9f};
    return {48,119,218,active?.72f:.48f,active?1.f:.8f};
}
}
