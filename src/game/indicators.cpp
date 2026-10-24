#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/guest_memory_guard.h"
#include "common/guest_test_memory.h"
#include "common/minimap_state.h"
#include "core/log.h"
#include <array>
#include <cstring>

X86_EXTERN_FUNC(__imp__sub_0035BA40);
X86_EXTERN_FUNC(__imp__sub_002FD450);
X86_EXTERN_FUNC(__imp__sub_002F67D0);
namespace {
template<class T> T Read(const uint8_t* base,uint32_t address) {T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
template<class T> void Write(uint8_t* base,uint32_t address,T value) {std::memcpy(base+address,&value,sizeof(T));}
}
// Modelo emcmzoneyaji/1137: somente a escala da seta, sem alterar outros efeitos.
X86_FUNC(sub_0035BA40) {
    const uint32_t stack=ctx.esp;
    if(!dev::ReadableGuestRange(base,stack,12) || Read<uint32_t>(base,stack)!=0x8AC5C) {
        __imp__sub_0035BA40(ctx,base);return;
    }
    const auto vector=Read<uint32_t>(base,stack+8);
    if(stack<64 || !dev::ReadableGuestRange(base,stack-64,64) || !dev::ReadableGuestRange(base,vector,16)) {
        __imp__sub_0035BA40(ctx,base);return;
    }
    auto scale=Read<std::array<float,4>>(base,vector);
    for(int axis=0;axis<3;++axis)scale[axis]*=.2f;
    // Argumentos e vetor temporarios ficam acima da pilha usada pelo original.
    ctx.esp=stack-64;
    std::memcpy(base+ctx.esp,base+stack,12);
    Write(base,ctx.esp+8,ctx.esp+16);Write(base,ctx.esp+16,scale);
    __imp__sub_0035BA40(ctx,base);
    ctx.esp=stack+4;
}
namespace dev {
bool VerifyIndicatorHooks() {
    GuestTestMemory arena;if(!arena.data())return false;
    auto* base=arena.data();constexpr uint32_t object=0x210000,vector=0x220000;
    Write(base,vector,std::array<float,4>{1.25f,1.25f,1.25f,1});
    const std::array<float,4> position{10,20,30,1};Write(base,object+0x80,position);
    X86Context call{};
    auto apply=[&](uint32_t caller) {
        call.esp=GuestTestMemory::stack;Write(base,call.esp,caller);
        Write(base,call.esp+4,object);Write(base,call.esp+8,vector);
        sub_0035BA40(call,base);return Read<std::array<float,4>>(base,object+0xA0);
    };
    const auto white=apply(0x8AC5C),other=apply(0x12345);
    const bool ok=white==std::array<float,4>{.25f,.25f,.25f,1} &&
        other==std::array<float,4>{1.25f,1.25f,1.25f,1} &&
        Read<std::array<float,4>>(base,object+0x80)==position &&
        Read<std::array<float,4>>(base,vector)==other && call.esp==GuestTestMemory::stack+4;
    LOGR("dev selftest: white indicator 20pct scale/position/source/caller guard=%d",ok);
    Write(base,0xB24640,uint32_t(0x250000));Write(base,0x25000C,uint32_t(0x260000));
    Write(base,0x260020,uint32_t(0x270000));Write(base,0x27003C,uint32_t(0x490));
    Write(base,0x548CC8,uint32_t(0x280000));Write(base,0x280009,uint8_t(1));Write(base,0x290000,uint32_t(9));
    call.esp=GuestTestMemory::stack;Write(base,call.esp+4,uint32_t(2));Write(base,call.esp+8,uint32_t(3));
    __imp__sub_002FD450(call,base);const auto native_type=call.eax;
    call.esp=GuestTestMemory::stack;Write(base,call.esp+4,uint32_t(9));
    __imp__sub_002F67D0(call,base);const auto native_visited=call.eax;
    auto read=[&](uint32_t address,auto& value) {
        if(!ReadableGuestRange(base,address,sizeof(value)))return false;
        std::memcpy(&value,base+address,sizeof(value));return true;
    };
    const auto room=cod_map::NativeRoomState(2,3,0x290000,1,read);
    const bool map_ok=native_type==0x490&&native_visited==1&&room.type==native_type&&room.explored;
    LOGR("dev selftest: minimap native room type/exploration getters=%d",map_ok);
    return ok&&map_ok;
}
}
