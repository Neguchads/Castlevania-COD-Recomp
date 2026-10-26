#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/game_cheat_rules.h"
#include "common/enemy_probe_rules.h"
#include "common/dev_rules.h"
#include "common/guest_test_memory.h"
#include <array>
#include <atomic>
#include <cstring>
#include <vector>
#include "core/log.h"

X86_EXTERN_FUNC(__imp__sub_00336070);
X86_EXTERN_FUNC(__imp__sub_00336310);
X86_EXTERN_FUNC(__imp__sub_00336680);
X86_EXTERN_FUNC(__imp__sub_00335D30);
X86_EXTERN_FUNC(__imp__sub_00337A00);
X86_EXTERN_FUNC(__imp__sub_002F6230);
X86_EXTERN_FUNC(__imp__sub_0031F270);
X86_EXTERN_FUNC(__imp__sub_00090660);

namespace {
// Dificuldade: fator do jogador (slot fisico 0) ou dos Innocent Devils (demais slots da raiz B2A350).
float DamageFactor(uint32_t slot) { return dev::ActorDamageFactor(slot,dev::damage_taken.load(),dev::devil_damage_taken.load()); }
bool DifficultyActive() { return dev::damage_taken.load()!=1.0f || dev::devil_damage_taken.load()!=1.0f; }
// Primeiros danos com slot e ID no log: mostra se algum ator fora do grupo aparece nesta raiz.
void LogDamage(const char* path,uint32_t id,uint32_t slot,int32_t before,int32_t after) {
    static std::atomic<int> logged{0};
    if(logged.fetch_add(1)<24) LOGR("difficulty: %s id=%u slot=%u damage %d -> %d",path,id,slot,before,after);
}
}
X86_FUNC(sub_00337A00) {
    if(dev::godmode_ready && dev::ReadableGuestRange(base,ctx.esp,20)) {
        auto read=[&](uint32_t address){uint32_t value;std::memcpy(&value,base+address,4);return value;};
        const auto root=read(ctx.esp+4),id=read(ctx.esp+8),stat=read(ctx.esp+12);
        const auto slot=id<60 ? read(0xB414E8+id*4) : ~0u;
        if(root==0xB2A350 && id<60 && stat==6 && slot==0 && dev::godmode.load()) {
            const auto hp=read(0xB2CCD8);
            const int32_t next=int32_t(read(ctx.esp+16));
            if(int32_t(hp)>0 && next<int32_t(hp)) {
                std::memcpy(base+ctx.esp+16,&hp,4);
                const auto blocked=dev::prevented_damage.fetch_add(1);
                if(blocked<12) LOGR("godmode: absolute damage blocked caller=%08X id=%u hp=%u requested=%d",read(ctx.esp),id,hp,next);
            }
        } else if(root==0xB2A350 && id<60 && stat==6 && slot<128 && DifficultyActive() &&
                  dev::ReadableGuestRange(base,0xB2CCD8+slot*0x238,4)) {
            const auto hp=int32_t(read(0xB2CCD8+slot*0x238));
            const int32_t next=int32_t(read(ctx.esp+16));
            const int32_t scaled=dev::ScaleAbsoluteHp(hp,next,DamageFactor(slot));
            if(scaled!=next) {
                std::memcpy(base+ctx.esp+16,&scaled,4);
                LogDamage("absolute",id,slot,hp-next,hp-scaled);
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
        } else if (dev::BlockMpCost(dev::infinite_mp.load(), ctx.eax, slot, delta)) {
            const int32_t zero = 0;
            std::memcpy(base + ctx.esp + 4, &zero, 4);
        } else if (ctx.eax == 6 && delta < 0 && DifficultyActive()) {
            const int32_t scaled = dev::ScaleDamageDelta(delta, DamageFactor(slot));
            if (scaled != delta) {
                std::memcpy(base + ctx.esp + 4, &scaled, 4);
                LogDamage("delta", ctx.ebx, slot, -delta, -scaled);
            }
        } else if (ctx.eax == dev::kStatGold || ctx.eax == dev::kStatExp) {
            // Ouro entra por moeda, venda e compra; EXP do jogador por sub_00336A60 (unico 336680 com stat 3).
            const dev::EconomyCheats cheats{dev::gold_gain.load(), dev::exp_gain.load(), dev::free_shopping.load()};
            const int32_t adjusted = dev::AdjustEconomyDelta(ctx.eax, slot, delta, cheats);
            if (adjusted != delta) {
                std::memcpy(base + ctx.esp + 4, &adjusted, 4);
                static std::atomic<int> logged{0};
                if (logged.fetch_add(1) < 16) LOGR("cheat: %s id=%u delta %d -> %d", ctx.eax == dev::kStatGold ? "gold" : "exp", ctx.ebx, delta, adjusted);
            }
        }
    }
    __imp__sub_00336070(ctx, base);
}

// Painel do inimigo (roda a cada quadro com o painel visivel): alvo em [0xB251F0], ficha em alvo+0x128.
// Com a sonda ligada, compara alvo e ficha com o quadro anterior e registra os inteiros que cairam.
X86_EXTERN_FUNC(__imp__sub_00309010);
X86_FUNC(sub_00309010) {
    if (dev::enemy_probe.load() && dev::ReadableGuestRange(base, 0xB251F0, 4)) {
        constexpr uint32_t kActorBytes = 0x400, kInfoBytes = 0x100;
        static uint32_t last_actor = 0, last_info = 0;
        static std::vector<uint8_t> actor_before, info_before;
        static int logged = 0;
        uint32_t actor = 0, info = 0;
        std::memcpy(&actor, base + 0xB251F0, 4);
        if (actor && dev::ReadableGuestRange(base, actor, kActorBytes)) {
            std::memcpy(&info, base + actor + 0x128, 4);
            const bool info_ok = info && dev::ReadableGuestRange(base, info, kInfoBytes);
            std::vector<uint8_t> actor_now(base + actor, base + actor + kActorBytes);
            std::vector<uint8_t> info_now;
            if (info_ok) info_now.assign(base + info, base + info + kInfoBytes);
            if (actor == last_actor && logged < 400) {
                for (const auto& drop : dev::DroppedFields(actor_before, actor_now))
                    if (logged++ < 400) LOGR("probe: actor %08X +%03X int%d %d -> %d", actor, drop.offset, drop.bits, drop.before, drop.after);
                if (info == last_info && info_ok)
                    for (const auto& drop : dev::DroppedFields(info_before, info_now))
                        if (logged++ < 400) LOGR("probe: info %08X +%03X int%d %d -> %d", info, drop.offset, drop.bits, drop.before, drop.after);
            } else if (actor != last_actor && logged < 400) {
                LOGR("probe: new target actor=%08X info=%08X", actor, info);
            }
            last_actor = actor; last_info = info;
            actor_before = std::move(actor_now); info_before = std::move(info_now);
        }
    }
    __imp__sub_00309010(ctx, base);
}

// Leitura de flag (cdecl, flag em [esp+4], valor em eax). Desligado, nem le a pilha.
X86_FUNC(sub_002F6230) {
    uint32_t flag = 0;
    const bool simulate = dev::lament_save.load() && dev::ReadableGuestRange(base, ctx.esp, 8);
    if (simulate) std::memcpy(&flag, base + ctx.esp + 4, 4);
    __imp__sub_002F6230(ctx, base);
    if (simulate && flag == dev::kFlagLamentSave) {
        const uint32_t value = dev::LamentSaveFlag(flag, ctx.eax, true);
        if (value != ctx.eax) {
            static std::atomic<int> logged{0};
            if (logged.fetch_add(1) < 4) LOGR("cheat: Lament of Innocence save simulated (flag %X)", flag);
        }
        ctx.eax = value;
    }
}

// Vida maxima do inimigo (cdecl, tipo em [esp+4], resultado no topo do x87). O nascimento copia
// para a vida atual [sub_000276B0(ator)+0x1C]; a barra divide uma pela outra, entao a proporcao continua certa.
X86_FUNC(sub_00090660) {
    // o ret do original tira o endereco de retorno: o tipo so pode ser lido antes
    uint32_t type = 0;
    if (dev::ReadableGuestRange(base, ctx.esp, 8)) std::memcpy(&type, base + ctx.esp + 4, 4);
    __imp__sub_00090660(ctx, base);
    const float factor = dev::enemy_hp.load();
    if (factor == 1.0f) return;
    double& top = ctx.st[ctx.top & 7];
    const double before = top;
    top = dev::EnemyMaxHp(before, factor);
    static std::atomic<int> logged{0};
    if (top != before && logged.fetch_add(1) < 8)
        LOGR("difficulty: enemy type %u max HP %.0f -> %.0f", type, before, top);
}

// Botoes do jogo depois da configuracao de botoes nativa (sub_002F6B50: so o direcional passa direto;
// os demais dependem da tabela B23618). 0x20 e a acao do slot 11, Back/mapa no padrao.
X86_FUNC(sub_0031F270) {
    __imp__sub_0031F270(ctx, base);
    static uint32_t last = 0;
    static std::atomic<int> logged{0};
    const uint32_t now = ctx.eax & 0xFFFF;
    if (dev_rules::RisingBit(last, now, 0x20) && logged.fetch_add(1) < 8)
        LOGR("input: game action 0x20 (Back/map) received buttons=%04X", now);
    last = now;
}

namespace dev {
// Botoes do menu: escrita direta, so com a assinatura validada (mesmos campos do godmode).
bool RefillPlayerHp() {
    constexpr uint32_t hp_address=0xB2CCD8;   // atributo 6 do slot 0; vida maxima (5) logo antes
    if(!godmode_ready || !ReadableGuestRange(kernel::g_base,hp_address-4,8)) return false;
    int32_t max_hp,hp;
    std::memcpy(&max_hp,kernel::g_base+hp_address-4,4);std::memcpy(&hp,kernel::g_base+hp_address,4);
    const int32_t next=RefillHp(hp,max_hp);
    if(next==hp) return false;
    std::memcpy(kernel::g_base+hp_address,&next,4);
    LOGR("cheat: HP %d -> %d",hp,next);
    return true;
}
bool RefillPlayerMp() {
    constexpr uint32_t mp_address=0xB2A350+0x2970+kStatMp*4;   // atributo 32 do slot 0; maximo (31) logo antes
    if(!godmode_ready || !ReadableGuestRange(kernel::g_base,mp_address-4,8)) return false;
    int32_t max_mp,mp;
    std::memcpy(&max_mp,kernel::g_base+mp_address-4,4);std::memcpy(&mp,kernel::g_base+mp_address,4);
    const int32_t next=RefillMp(mp,max_mp);
    if(next==mp) return false;
    std::memcpy(kernel::g_base+mp_address,&next,4);
    LOGR("cheat: MP %d -> %d",mp,next);
    return true;
}
bool MaxPlayerGold() {
    constexpr uint32_t gold_address=0xB2A350+0xAEB4;
    if(!godmode_ready || !ReadableGuestRange(kernel::g_base,gold_address,4)) return false;
    std::memcpy(kernel::g_base+gold_address,&kMaxGold,4);
    LOGR("cheat: gold set to %d",kMaxGold);
    return true;
}

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
    // Economia pelo setter original: ouro em raiz+0xAEB4, EXP no atributo 3 do slot 0.
    const float saved_gold=gold_gain.load(),saved_exp=exp_gain.load();const bool saved_free=free_shopping.load();
    auto economy=[&](uint32_t stat,uint32_t address,uint32_t initial,int32_t delta,float gold,float exp,bool free){
        mapped(false,1,0);
        write(address,initial);
        gold_gain.store(gold);exp_gain.store(exp);free_shopping.store(free);
        X86Context context{};
        // mapped() poe o ID 1 no slot 0, onde fica o EXP lido abaixo.
        context.esp=GuestTestMemory::stack;context.eax=stat;context.ebx=1;context.edx=0xB2A350;
        write(context.esp+4,uint32_t(delta));
        sub_00336070(context,arena.data());
        return read(address);
    };
    constexpr uint32_t gold=0xB35204,exp=0xB2CCCC;
    const auto gold_off=economy(kStatGold,gold,1000,100,1,1,false),gold_on=economy(kStatGold,gold,1000,100,3,1,false);
    const auto spend_off=economy(kStatGold,gold,1000,-500,1,1,false),spend_free=economy(kStatGold,gold,1000,-500,1,1,true);
    const auto gold_cap=economy(kStatGold,gold,kMaxGold-10,100,3,1,false);
    const auto exp_off=economy(kStatExp,exp,500,40,1,1,false),exp_on=economy(kStatExp,exp,500,40,1,2,false);
    const bool economy_ok=gold_off==1100 && gold_on==1300 && spend_off==500 && spend_free==1000 &&
        gold_cap==uint32_t(kMaxGold) && exp_off==540 && exp_on==580;
    LOGR("dev selftest: economy gold off=%u x3=%u spend=%u free=%u cap=%u exp off=%u x2=%u passed=%d",
         gold_off,gold_on,spend_off,spend_free,gold_cap,exp_off,exp_on,economy_ok);
    gold_gain.store(saved_gold);exp_gain.store(saved_exp);free_shopping.store(saved_free);
    ok=economy_ok && ok;
    // MP infinito pelo setter original: atributo 32 do slot 0 em raiz+0x2970+32*4.
    const bool saved_mp=infinite_mp.load();
    auto mp=[&](bool enabled,int32_t delta){
        mapped(false,1,0);
        constexpr uint32_t address=0xB2A350+0x2970+kStatMp*4;
        write(address-4,80);write(address,50);
        infinite_mp.store(enabled);
        X86Context context{};
        context.esp=GuestTestMemory::stack;context.eax=kStatMp;context.ebx=1;context.edx=0xB2A350;
        write(context.esp+4,uint32_t(delta));
        sub_00336070(context,arena.data());
        return read(address);
    };
    const auto mp_off=mp(false,-10),mp_on=mp(true,-10),mp_gain=mp(true,10);
    const bool mp_ok=mp_off==40 && mp_on==50 && mp_gain==60;
    LOGR("dev selftest: infinite mp off=%u on=%u gain=%u passed=%d",mp_off,mp_on,mp_gain,mp_ok);
    infinite_mp.store(saved_mp);
    ok=mp_ok && ok;
    // Save de LoI pelo leitor original: grupo 0 em [0x548C0C], palavra 1, bit 2 = flag 0x4002.
    const bool saved_lament=lament_save.load();
    auto flag=[&](uint32_t id,uint32_t word,bool simulate){
        write(0x548C0C,0x600000);write(0x600004,word);
        lament_save.store(simulate);
        X86Context context{};context.esp=GuestTestMemory::stack;
        write(context.esp+4,id);
        sub_002F6230(context,arena.data());
        return context.eax;
    };
    const auto lament_off=flag(kFlagLamentSave,0,false),lament_on=flag(kFlagLamentSave,0,true);
    const auto lament_real=flag(kFlagLamentSave,4,false),other_on=flag(0x4001,0,true),other_set=flag(0x4001,2,true);
    const bool lament_ok=lament_off==0 && lament_on==1 && lament_real==1 && other_on==0 && other_set==1;
    LOGR("dev selftest: lament save off=%u on=%u real=%u other=%u other-set=%u passed=%d",
         lament_off,lament_on,lament_real,other_on,other_set,lament_ok);
    lament_save.store(saved_lament);
    ok=lament_ok && ok;
    LOGR("dev selftest: godmode virtual-stack=%08X passed=%d",GuestTestMemory::stack,ok);
    godmode.store(saved);prevented_damage.store(saved_counter);
    return ok;
}
}
