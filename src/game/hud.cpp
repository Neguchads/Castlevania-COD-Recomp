#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/guest_memory_guard.h"
#include "common/presentation_math.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <array>
#include <cstring>

X86_EXTERN_FUNC(__imp__sub_003575B0);
X86_EXTERN_FUNC(__imp__sub_00357610);
X86_EXTERN_FUNC(__imp__sub_003094E0);
X86_EXTERN_FUNC(__imp__sub_00308E70);
X86_EXTERN_FUNC(__imp__sub_00367140);
X86_EXTERN_FUNC(__imp__sub_003677F0);
X86_EXTERN_FUNC(__imp__sub_002F6230);
namespace {
template<class T> T Read(const uint8_t* base,uint32_t address) {T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
template<class T> void Write(uint8_t* base,uint32_t address,T value) {std::memcpy(base+address,&value,sizeof(T));}
bool HudObject(uint8_t* base,uint32_t object) {
    if(!dev::ReadableGuestRange(base,0xB251F0,4)) return false;
    const auto root=Read<uint32_t>(base,0xB251F0);
    for(int depth=0;root && object && depth<16;++depth) {
        if(object==root) return true;
        if(!dev::ReadableGuestRange(base,object,0x12C)) return false;
        object=Read<uint32_t>(base,object+0x40);
    }
    return false;
}
thread_local unsigned drawing=0;
template<class Original> void DrawSprite(X86Context& ctx,uint8_t* base,Original original) {
    if(drawing || !dev::ReadableGuestRange(base,ctx.esp,8)) {original(ctx,base);return;}
    const auto object=Read<uint32_t>(base,ctx.esp+4);
    const bool hud=HudObject(base,object);
    constexpr bool menu=false;
    if((!hud&&!menu) || !dev::ReadableGuestRange(base,object+0x128,4)) {original(ctx,base);return;}
    if(hud&&!dev::hud_enabled){ctx.esp+=4;return;}
    const auto data=Read<uint32_t>(base,object+0x128);
    if(!dev::ReadableGuestRange(base,data,0x4C)){original(ctx,base);return;}
    const auto xy=Read<uint32_t>(base,data),offset=Read<uint32_t>(base,data+0x40);
    const auto sx=Read<float>(base,data+8),sy=Read<float>(base,data+12);
    const float scale=hud ? dev::hud_scale.load() : dev::game_menu_scale.load();
    const auto x=Read<int16_t>(base,data)+Read<int16_t>(base,data+0x40);
    const auto y=Read<int16_t>(base,data+2)+Read<int16_t>(base,data+0x42);
    Write(base,data,int16_t(std::round(hud ? cod_view::HudCoordinate(float(x),scale,true) : x*scale)));
    Write(base,data+2,int16_t(std::round(hud ? cod_view::HudCoordinate(float(y),scale,false) : y*scale)));
    Write(base,data+0x40,uint32_t(0));Write(base,data+8,sx*scale);Write(base,data+12,sy*scale);
    ++drawing;original(ctx,base);--drawing;
    Write(base,data,xy);Write(base,data+0x40,offset);Write(base,data+8,sx);Write(base,data+12,sy);
}
}
X86_FUNC(sub_003575B0) {DrawSprite(ctx,base,__imp__sub_003575B0);}
X86_FUNC(sub_00357610) {DrawSprite(ctx,base,__imp__sub_00357610);}
X86_FUNC(sub_00308E70) {
    if(!dev::hud_enabled){ctx.esp+=4;return;}
    __imp__sub_00308E70(ctx,base);
}
X86_FUNC(sub_003094E0) {
    const bool quick=dev::hud_quick_hide && dev::ReadableGuestRange(base,ctx.esp,8) && Read<uint32_t>(base,ctx.esp+4)==1;
    __imp__sub_003094E0(ctx,base);
    if(quick && dev::ReadableGuestRange(base,0xB251F0,4)) {
        const auto object=Read<uint32_t>(base,0xB251F0);
        if(!object || !dev::ReadableGuestRange(base,object+0x128,4)) return;
        const auto data=Read<uint32_t>(base,object+0x128);
        if(data && dev::ReadableGuestRange(base,data+0x4C,4)) Write(base,data+0x4C,dev::hud_hide_delay.load());
    }
}
// Somente o texto do Innocent Devil no HUD: escala de layout e ancora da chamada original.
X86_FUNC(sub_00367140) {
    if(!dev::ReadableGuestRange(base,ctx.esp,24)) {__imp__sub_00367140(ctx,base);return;}
    const auto caller=Read<uint32_t>(base,ctx.esp);
    const bool hud=caller==0x308F02;
    constexpr bool menu=false;
    if(!hud&&!menu){__imp__sub_00367140(ctx,base);return;}
    const auto stack=ctx.esp;const float x=Read<float>(base,stack+16),y=Read<float>(base,stack+20);
    const float scale=hud ? dev::hud_scale.load() : dev::game_menu_scale.load();
    Write(base,stack+16,x*scale);Write(base,stack+20,y*scale);
    __imp__sub_00367140(ctx,base);Write(base,stack+16,x);Write(base,stack+20,y);
}
X86_FUNC(sub_003677F0) {
    if(!dev::ReadableGuestRange(base,ctx.esp,20)) {__imp__sub_003677F0(ctx,base);return;}
    const auto caller=Read<uint32_t>(base,ctx.esp);const bool hud=caller==0x308F48;
    constexpr bool menu=false;
    if(!hud&&!menu){__imp__sub_003677F0(ctx,base);return;}
    const auto stack=ctx.esp;const auto x=Read<int32_t>(base,stack+12),y=Read<int32_t>(base,stack+16);
    const float scale=hud ? dev::hud_scale.load() : dev::game_menu_scale.load();
    Write(base,stack+12,int32_t(std::round(hud ? cod_view::HudCoordinate(float(x),scale,true) : x*scale)));
    Write(base,stack+16,int32_t(std::round(hud ? cod_view::HudCoordinate(float(y),scale,false) : y*scale)));
    __imp__sub_003677F0(ctx,base);Write(base,stack+12,x);Write(base,stack+16,y);
}
namespace dev {
bool NativeMenuActive(uint8_t* base) {
    return ReadableGuestRange(base,0xB25518,4) && Read<uint32_t>(base,0xB25518)!=0;
}
bool VerifyHudHooks() {
    GuestTestMemory arena;if(!arena.data() || !kernel::g_base) return false;
    auto* memory=arena.data();
    constexpr uint32_t root=0x210000,sprite=0x220000,data=0x230000;
    Write(memory,0xB251F0,root);Write(memory,sprite+0x40,root);Write(memory,sprite+0x128,data);
    Write(memory,root+0x128,data);Write(memory,root+0x130,uint32_t(0x30));
    Write(memory,data,int16_t(-200));Write(memory,data+2,int16_t(200));
    Write(memory,data+8,1.0f);Write(memory,data+12,2.0f);Write(memory,data+0x40,uint32_t(0));
    const bool enabled=hud_enabled.load(),quick=hud_quick_hide.load();const float scale=hud_scale.load(),delay=hud_hide_delay.load();
    hud_enabled=true;hud_scale=.5f;
    bool called=false,transformed=false;
    auto draw=[&](X86Context& call,uint8_t* bytes) {
        called=true;transformed=Read<int16_t>(bytes,data)==-228 && Read<int16_t>(bytes,data+2)==212 &&
            Read<float>(bytes,data+8)==.5f && Read<float>(bytes,data+12)==1;
        call.esp+=4;
    };
    X86Context call{};call.esp=GuestTestMemory::stack;Write(memory,call.esp+4,sprite);
    DrawSprite(call,memory,draw);
    bool ok=called && transformed && call.esp==GuestTestMemory::stack+4 && Read<int16_t>(memory,data)==-200 &&
        Read<int16_t>(memory,data+2)==200 && Read<float>(memory,data+8)==1 && Read<float>(memory,data+12)==2;
    hud_enabled=false;called=false;call.esp=GuestTestMemory::stack;DrawSprite(call,memory,draw);
    ok=ok && !called && call.esp==GuestTestMemory::stack+4;
    Write(memory,sprite+0x40,uint32_t(0));called=false;call.esp=GuestTestMemory::stack;DrawSprite(call,memory,draw);
    ok=ok && called;
    const float saved_menu_scale=game_menu_scale.load();game_menu_scale=.4f;
    Write(memory,0xB25518,root);Write(memory,0xB251F0,uint32_t(0));Write(memory,sprite+0x40,root);
    call.esp=GuestTestMemory::stack;
    bool menu_scaled=false;
    DrawSprite(call,memory,[&](X86Context& context,uint8_t* bytes) {
        menu_scaled=Read<int16_t>(bytes,data)==-200 && Read<int16_t>(bytes,data+2)==200 && Read<float>(bytes,data+8)==1;
        context.esp+=4;
    });
    ok=ok&&menu_scaled&&Read<int16_t>(memory,data)==-200&&Read<float>(memory,data+8)==1;
    game_menu_scale=saved_menu_scale;Write(memory,0xB251F0,root);
    std::memcpy(memory+0x309548,kernel::g_base+0x309548,16);
    std::memcpy(memory+0x432ADC,kernel::g_base+0x432ADC,4);
    std::memcpy(memory+0x430CE0,kernel::g_base+0x430CE0,4);
    auto timer=[&](bool fast,int mode) {
        hud_quick_hide=fast;hud_hide_delay=.6f;call.esp=GuestTestMemory::stack;Write(memory,call.esp+4,uint32_t(mode));
        sub_003094E0(call,memory);return Read<float>(memory,data+0x4C);
    };
    const float normal=timer(false,1),fast=timer(true,1),fade=timer(true,2);
    ok=ok && normal==5 && fast==.6f && std::abs(fade-.3f)<.001f;
    Write(memory,0x548C10,uint32_t(0x250000));
    auto modal=[&](uint32_t flags) {
        Write(memory,0x250000,flags);X86Context query{};query.esp=GuestTestMemory::stack;
        Write(memory,query.esp+4,uint32_t(0x10000010));__imp__sub_002F6230(query,memory);return query.eax;
    };
    ok=ok && modal(0)==0 && modal(1u<<16)==1 && modal(1u<<19)==0;
    hud_enabled=enabled;hud_scale=scale;hud_quick_hide=quick;hud_hide_delay=delay;
    LOGR("dev selftest: HUD hook scale/restore/hide/menu-guard=%d timer=%.1f/%.1f fade=%.1f",ok,normal,fast,fade);
    LOGR("dev selftest: native menu original layout preserved=%d",menu_scaled);
    return ok;
}
}
