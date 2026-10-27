#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/guest_memory_guard.h"
#include "common/presentation_math.h"
#include "common/hud_render_rules.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <array>
#include <cstring>

X86_EXTERN_FUNC(__imp__sub_003575B0);
X86_EXTERN_FUNC(__imp__sub_00357610);
X86_EXTERN_FUNC(__imp__sub_003094E0);
X86_EXTERN_FUNC(__imp__sub_00309930);
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
    // O painel de comandos reutiliza um sprite temporario fora da arvore do HUD.
    const auto caller=Read<uint32_t>(base,ctx.esp);
    const bool pet=caller==0x308F97 || caller==0x308FD1 || caller==0x308FFD;
    const bool life=caller==0x308CAD || caller==0x308CEA || caller==0x308D18 || caller==0x308D56;
    const bool enemy=caller==0x309171 || caller==0x3091AE || caller==0x3091D8;
    const bool hud=pet || life || enemy || HudObject(base,object);
    constexpr bool menu=false;
    if((!hud&&!menu) || !dev::ReadableGuestRange(base,object+0x128,4)) {original(ctx,base);return;}
    if(hud&&!dev::hud_enabled){ctx.esp+=4;return;}
    const auto data=Read<uint32_t>(base,object+0x128);
    if(!dev::ReadableGuestRange(base,data,0x5A)){original(ctx,base);return;}
    const auto xy=Read<uint32_t>(base,data),offset=Read<uint32_t>(base,data+0x40);
    const auto sx=Read<float>(base,data+8),sy=Read<float>(base,data+12);
    const float scale=hud ? dev::hud_scale.load() : dev::game_menu_scale.load();
    const auto x=Read<int16_t>(base,data)+Read<int16_t>(base,data+0x40);
    const auto y=Read<int16_t>(base,data+2)+Read<int16_t>(base,data+0x42);
    Write(base,data,int16_t(std::round(hud ? (enemy ? cod_view::EnemyPanelCoordinate(float(x),scale) : cod_view::HudCoordinate(float(x),scale,true)) : x*scale)));
    Write(base,data+2,int16_t(std::round(hud ? cod_view::HudCoordinate(float(y),scale,false) : y*scale)));
    Write(base,data+0x40,uint32_t(0));Write(base,data+8,sx*scale);Write(base,data+12,sy*scale);
    // Mesmo bit usado por357B30/357BE0: invalidar os vertices em cache.
    Write(base,data+0x59,uint8_t(Read<uint8_t>(base,data+0x59)|4));
    ++drawing;original(ctx,base);--drawing;
    Write(base,data,xy);Write(base,data+0x40,offset);Write(base,data+8,sx);Write(base,data+12,sy);
    Write(base,data+0x59,uint8_t(Read<uint8_t>(base,data+0x59)|4));
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
X86_FUNC(sub_00309930) {
    __imp__sub_00309930(ctx,base);
    if(!cod_hud::KeepVisible(dev::hud_enabled.load(),dev::hud_always_visible.load(),false))return;
    if(dev::NativeMenuActive(base))return;
    if(!dev::ReadableGuestRange(base,0xB251F0,4))return;
    const auto root=Read<uint32_t>(base,0xB251F0);
    if(!root || !dev::ReadableGuestRange(base,root+0x128,4))return;
    const auto data=Read<uint32_t>(base,root+0x128);
    if(!data || !dev::ReadableGuestRange(base,data,0x84))return;
    float timer=Read<float>(base,data+0x4C);
    auto alpha=Read<uint8_t>(base,data+0x7F);auto state=Read<uint32_t>(base,data+0x80);
    cod_hud::RefreshVisibility(timer,alpha,state);
    Write(base,data+0x4C,timer);Write(base,data+0x7F,alpha);Write(base,data+0x80,state);
}
// Somente o texto do Innocent Devil no HUD: escala de layout e ancora da chamada original.
X86_FUNC(sub_00367140) {
    if(!dev::ReadableGuestRange(base,ctx.esp,24)) {__imp__sub_00367140(ctx,base);return;}
    const auto caller=Read<uint32_t>(base,ctx.esp);
    const bool hud=caller==0x308F02 || caller==0x308BAC || caller==0x308C29 ||
        caller==0x309079 || caller==0x3090DD || caller==0x309201;
    constexpr bool menu=false;
    if(!hud&&!menu){__imp__sub_00367140(ctx,base);return;}
    const auto stack=ctx.esp;const float x=Read<float>(base,stack+16),y=Read<float>(base,stack+20);
    const float scale=hud ? dev::hud_scale.load() : dev::game_menu_scale.load();
    Write(base,stack+16,x*scale);Write(base,stack+20,y*scale);
    __imp__sub_00367140(ctx,base);Write(base,stack+16,x);Write(base,stack+20,y);
}
X86_FUNC(sub_003677F0) {
    if(!dev::ReadableGuestRange(base,ctx.esp,20)) {__imp__sub_003677F0(ctx,base);return;}
    const auto caller=Read<uint32_t>(base,ctx.esp);const bool enemy=caller==0x3090B3 || caller==0x309112 || caller==0x309232;
    const bool hud=enemy || caller==0x308F48 || caller==0x308BDE || caller==0x308C5C;
    constexpr bool menu=false;
    if(!hud&&!menu){__imp__sub_003677F0(ctx,base);return;}
    const auto stack=ctx.esp;const auto x=Read<int32_t>(base,stack+12),y=Read<int32_t>(base,stack+16);
    const float scale=hud ? dev::hud_scale.load() : dev::game_menu_scale.load();
    Write(base,stack+12,int32_t(std::round(hud ? (enemy ? cod_view::EnemyPanelCoordinate(float(x),scale) : cod_view::HudCoordinate(float(x),scale,true)) : x*scale)));
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
        called=true;transformed=(Read<uint8_t>(bytes,data+0x59)&4)!=0 && Read<int16_t>(bytes,data)==-228 && Read<int16_t>(bytes,data+2)==212 &&
            Read<float>(bytes,data+8)==.5f && Read<float>(bytes,data+12)==1;
        Write(bytes,data+0x59,uint8_t(Read<uint8_t>(bytes,data+0x59)&~4u));
        call.esp+=4;
    };
    X86Context call{};call.esp=GuestTestMemory::stack;Write(memory,call.esp+4,sprite);
    DrawSprite(call,memory,draw);
    bool ok=called && transformed && call.esp==GuestTestMemory::stack+4 && Read<int16_t>(memory,data)==-200 &&
        Read<int16_t>(memory,data+2)==200 && Read<float>(memory,data+8)==1 && Read<float>(memory,data+12)==2 && (Read<uint8_t>(memory,data+0x59)&4)!=0;
    hud_enabled=false;called=false;call.esp=GuestTestMemory::stack;DrawSprite(call,memory,draw);
    ok=ok && !called && call.esp==GuestTestMemory::stack+4;
    Write(memory,sprite+0x40,uint32_t(0));called=false;call.esp=GuestTestMemory::stack;DrawSprite(call,memory,draw);
    ok=ok && called;
    bool pet_ok=true;
    hud_enabled=true;
    for(const uint32_t caller:{0x308F97u,0x308FD1u,0x308FFDu,0x308CADu,0x308CEAu,0x308D18u,0x308D56u}) {
        for(const float factor:{1.0f,.5f,.35f}) {
            hud_scale=factor;call.esp=GuestTestMemory::stack;
            Write(memory,call.esp,caller);
            bool scaled=false;
            DrawSprite(call,memory,[&](X86Context& context,uint8_t* bytes) {
                scaled=Read<int16_t>(bytes,data)==int16_t(std::round(cod_view::HudCoordinate(-200,factor,true))) &&
                    Read<int16_t>(bytes,data+2)==int16_t(std::round(cod_view::HudCoordinate(200,factor,false))) &&
                    Read<float>(bytes,data+8)==factor && Read<float>(bytes,data+12)==2*factor;
                context.esp+=4;
            });
            pet_ok=pet_ok && scaled && call.esp==GuestTestMemory::stack+4 &&
                Read<int16_t>(memory,data)==-200 && Read<int16_t>(memory,data+2)==200 &&
                Read<float>(memory,data+8)==1 && Read<float>(memory,data+12)==2;
        }
        hud_enabled=false;called=false;call.esp=GuestTestMemory::stack;
        DrawSprite(call,memory,draw);pet_ok=pet_ok && !called;
        hud_enabled=true;
    }
    for(const uint32_t caller:{0x309171u,0x3091AEu,0x3091D8u}) {
        for(const float factor:{1.f,.5f,.35f}) {
            call.esp=GuestTestMemory::stack;Write(memory,call.esp,caller);hud_scale=factor;
            bool scaled=false;
            DrawSprite(call,memory,[&](X86Context& context,uint8_t* bytes) {
                scaled=Read<int16_t>(bytes,data)==int16_t(std::round(cod_view::EnemyPanelCoordinate(-200,factor))) &&
                    Read<float>(bytes,data+8)==factor;
                context.esp+=4;
            });
            pet_ok=pet_ok && scaled && Read<int16_t>(memory,data)==-200 && Read<float>(memory,data+8)==1;
        }
    }
    Write(memory,GuestTestMemory::stack,uint32_t(0));hud_scale=.5f;
    ok=ok && pet_ok;
    LOGR("dev selftest: pet scratch sprite scale/restore/hide=%d",pet_ok);
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
