#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/game_cheat_rules.h"
#include "common/guest_test_memory.h"
#include <array>
#include <cstring>
#include <vector>
#include "core/log.h"

X86_EXTERN_FUNC(__imp__sub_00336070);
X86_EXTERN_FUNC(__imp__sub_00336310);
X86_EXTERN_FUNC(__imp__sub_00336680);
X86_EXTERN_FUNC(__imp__sub_00335D30);
X86_EXTERN_FUNC(__imp__sub_00337A00);

X86_FUNC(sub_00337A00) {
    if(dev::godmode_ready && dev::godmode.load() && dev::ReadableGuestRange(base,ctx.esp,20)) {
        auto read=[&](uint32_t address){uint32_t value;std::memcpy(&value,base+address,4);return value;};
        const auto root=read(ctx.esp+4),id=read(ctx.esp+8),stat=read(ctx.esp+12);
        if(root==0xB2A350 && id<60 && stat==6 && read(0xB414E8+id*4)==0) {
            const auto hp=read(0xB2CCD8);
            const int32_t next=int32_t(read(ctx.esp+16));
            if(int32_t(hp)>0 && next<int32_t(hp)) {
                std::memcpy(base+ctx.esp+16,&hp,4);
                const auto blocked=dev::prevented_damage.fetch_add(1);
                if(blocked<12) LOGR("godmode: absolute damage blocked caller=%08X id=%u hp=%u requested=%d",read(ctx.esp),id,hp,next);
            }
        }
    }
    __imp__sub_00337A00(ctx,base);
}

X86_FUNC(sub_00336070) {
    // EBX e ID logico: so o mapa do jogo identifica o slot fisico do jogador.
    if (dev::godmode_ready && ctx.edx == 0xB2A350 && ctx.ebx < 60 &&
        dev::ReadableGuestRange(base,ctx.esp,8)) {
        int32_t delta;
        std::memcpy(&delta, base + ctx.esp + 4, 4);
        uint32_t slot;
        std::memcpy(&slot, base + 0xB414E8 + ctx.ebx * 4, 4);
        if (dev::BlockPlayerDamage(dev::godmode.load(), ctx.eax, slot, delta)) {
            const int32_t zero = 0;
            std::memcpy(base + ctx.esp + 4, &zero, 4);
            dev::prevented_damage.fetch_add(1);
        }
    }
    __imp__sub_00336070(ctx, base);
}

namespace dev {
bool ValidateGodmodeImage() {
    constexpr std::array<uint8_t,17> setter{0x8D,0xB4,0x82,0x70,0x29,0,0,0x01,0x2E,0x79,0x06,0xC7,0x06,0,0,0,0};
    constexpr std::array<uint8_t,8> actor{0x8B,0x04,0x85,0xE8,0x14,0xB4,0,0xC3};
    constexpr std::array<uint8_t,13> absolute{0x8B,0x54,0x24,0x04,0x56,0x8B,0x74,0x24,0x0C,0x57,0x56,0x33,0xC9};
    if (!kernel::g_base) return false;
    const bool valid = std::memcmp(kernel::g_base + 0x336113,setter.data(),setter.size()) == 0 &&
        std::memcmp(kernel::g_base + 0x335F80,actor.data(),actor.size()) == 0 &&
        std::memcmp(kernel::g_base + 0x337A00,absolute.data(),absolute.size()) == 0;
    godmode_ready.store(valid);
    if (!valid) godmode.store(false);
    return valid;
}

bool VerifyGodmodeOriginal() {
    if (!godmode_ready) return false;
    GuestTestMemory arena;
    if (!arena.data()) return false;
    std::memcpy(arena.data()+0x3361A8,kernel::g_base+0x3361A8,0x336270-0x3361A8);
    auto write = [&](uint32_t address,uint32_t value) { std::memcpy(arena.data()+address,&value,4); };
    auto read = [&](uint32_t address) { uint32_t value;std::memcpy(&value,arena.data()+address,4);return value; };
    write(0xB414E8,0);write(0xB414EC,1);
    const bool saved = godmode.load();
    const unsigned saved_counter = prevented_damage.load();
    auto run = [&](bool enabled,uint32_t actor,int32_t delta) {
        constexpr uint32_t hp = 0xB2CCD8;
        write(hp-4,120);write(hp,100);
        write(hp+0x238-4,120);write(hp+0x238,100);
        X86Context context{};
        context.esp=GuestTestMemory::stack;context.eax=6;context.ebx=actor;context.edx=0xB2A350;
        write(context.esp+4,uint32_t(delta));
        godmode.store(enabled);
        sub_00336070(context,arena.data());
        return read(hp+actor*0x238);
    };
    bool ok = run(false,0,-10)==90;
    ok = (run(true,0,-10)==100) && ok;
    ok = (run(true,0,10)==110) && ok;
    ok = (run(true,1,-10)==90) && ok;
    ok = (run(false,0,-10)==90) && ok;
    // O jogo monta o mapa por ID logico; ID 1 tambem pode ocupar o slot do jogador.
    auto mapped = [&](bool enabled, uint32_t id, int32_t delta, uint32_t player_id=1) {
        constexpr uint32_t root = 0xB2A350, hp = 0xB2CCD8;
        write(root+0xAEB8,2);
        write(root+0x2974,player_id);write(root+0x2974+0x238,player_id==1 ? 0 : 2);
        write(hp-4,120);write(hp,100);
        write(hp+0x238-4,120);write(hp+0x238,100);
        X86Context rebuild{};rebuild.esp=GuestTestMemory::stack;rebuild.edx=root;
        __imp__sub_00336310(rebuild,arena.data());
        X86Context call{};call.esp=GuestTestMemory::stack;
        write(call.esp+4,id);write(call.esp+8,6);write(call.esp+12,uint32_t(delta));
        godmode.store(enabled);
        __imp__sub_00336680(call,arena.data());
        return read(hp+read(0xB414E8+id*4)*0x238);
    };
    const auto mapped_off=mapped(false,1,-10);
    const auto mapped_on=mapped(true,1,-10);
    const auto mapped_heal=mapped(true,1,10);
    const auto mapped_other=mapped(true,0,-10);
    const auto mapped_zero=mapped(true,0,-10,0);
    const bool mapped_ok=mapped_off==90 && mapped_on==100 && mapped_heal==110 && mapped_other==90 && mapped_zero==100;
    LOGR("dev selftest: godmode mapped original off=%u on=%u heal=%u other=%u id0=%u passed=%d",
         mapped_off,mapped_on,mapped_heal,mapped_other,mapped_zero,mapped_ok);
    ok=mapped_ok && ok;
    std::memcpy(arena.data()+0x337B64,kernel::g_base+0x337B64,0x337C00-0x337B64);
    auto absolute=[&](bool enabled,uint32_t id,uint32_t value,uint32_t initial=100){
        mapped(false,1,0);
        write(0xB2CCD8,initial);
        X86Context call{};call.esp=GuestTestMemory::stack;
        write(call.esp+4,id);write(call.esp+8,6);write(call.esp+12,value);
        godmode.store(enabled);
        __imp__sub_00335D30(call,arena.data());
        return read(0xB2CCD8+read(0xB414E8+id*4)*0x238);
    };
    const auto absolute_off=absolute(false,1,90),absolute_on=absolute(true,1,90);
    const auto absolute_heal=absolute(true,1,110),absolute_other=absolute(true,0,90);
    const bool absolute_ok=absolute_off==90 && absolute_on==100 && absolute_heal==110 && absolute_other==90 &&
        absolute(true,1,0,0)==0 && absolute(true,1,0)==100 && absolute(false,1,0)==0;
    LOGR("dev selftest: godmode absolute off=%u on=%u heal=%u other=%u passed=%d",absolute_off,absolute_on,absolute_heal,absolute_other,absolute_ok);
    ok=absolute_ok && ok;
    LOGR("dev selftest: godmode virtual-stack=%08X passed=%d",GuestTestMemory::stack,ok);
    godmode.store(saved);prevented_damage.store(saved_counter);
    return ok;
}
}
