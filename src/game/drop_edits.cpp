#include "game/enemy_edits.h"
#include "common/dev_runtime.h"
#include "common/drop_rules.h"
#include "common/guest_memory_guard.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <atomic>
#include <cstring>
#include <mutex>
#include <random>

X86_EXTERN_FUNC(__imp__sub_00032D90);
X86_EXTERN_FUNC(__imp__sub_002F95E0);
X86_EXTERN_FUNC(__imp__sub_002F9A30);
X86_EXTERN_FUNC(__imp__sub_002F92F0);
namespace {
template<class T> T Read(const uint8_t* base,uint32_t address) {T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
template<class T> void Write(uint8_t* base,uint32_t address,T value) {std::memcpy(base+address,&value,sizeof(T));}
// Na entrada do hook, [esp] e o endereco de retorno e [esp+4] o primeiro argumento.
uint32_t ReturnAddress(const uint8_t* base,uint32_t esp) {
    return dev::ReadableGuestRange(base,esp,4) ? Read<uint32_t>(base,esp) : 0;
}
float Uniform() {
    static std::mutex lock;
    static std::mt19937 rng{std::random_device{}()};
    std::lock_guard guard(lock);
    return std::uniform_real_distribution<float>(0.0f,1.0f)(rng);
}
std::atomic<int> logged{0};
// Cristais: [esp+8] = quantidade que sub_002F95E0 reparte em ate 4 cristais.
bool AdjustCrystalAmount(uint8_t* base,uint32_t esp) {
    if(!dev::ReadableGuestRange(base,esp,12)) return false;
    const auto amount=Read<int32_t>(base,esp+8),next=dev::CrystalAmount(amount,dev::crystal_amount.load());
    if(next==amount) return false;
    Write(base,esp+8,next);
    if(logged.fetch_add(1)<16) LOGR("drops: crystal type %d amount %d -> %d",Read<int32_t>(base,esp+4),amount,next);
    return true;
}
// Itens aleatorios: troca [esp+4] por outro item da tabela de drops do proprio jogo.
bool RandomizeDropItem(uint8_t* base,uint32_t esp,float u) {
    if(!dev::random_drops.load() || !dev::ReadableGuestRange(base,esp,8) ||
       !dev::ReadableGuestRange(base,dev::kDropTable,dev::kDropTableEntries*2)) return false;
    const auto item=Read<int32_t>(base,esp+4);
    if(item<=0) return false;
    int16_t table[dev::kDropTableEntries];
    std::memcpy(table,base+dev::kDropTable,sizeof(table));
    const int32_t next=dev::PickDropItem(table,dev::kDropTableEntries,u,item);
    Write(base,esp+4,next);
    if(logged.fetch_add(1)<16) LOGR("drops: random item %d -> %d",item,next);
    return true;
}
}
namespace dev {
bool VerifyDropHooks() {
    GuestTestMemory arena;if(!arena.data())return false;
    auto* memory=arena.data();constexpr uint32_t esp=GuestTestMemory::stack;
    const float saved_amount=crystal_amount.load();const bool saved_random=random_drops.load();
    // Cristais 2x: quantidade 3 vira 6; 1x nao mexe.
    Write(memory,esp+4,int32_t(1));Write(memory,esp+8,int32_t(3));
    crystal_amount=2;const bool doubled=AdjustCrystalAmount(memory,esp) && Read<int32_t>(memory,esp+8)==6;
    crystal_amount=1;const bool neutral=!AdjustCrystalAmount(memory,esp) && Read<int32_t>(memory,esp+8)==6;
    // Item aleatorio vem da tabela: so a entrada 5 e valida.
    for(uint32_t i=0;i<kDropTableEntries;++i) Write(memory,kDropTable+i*2,int16_t(i==5 ? 77 : 0));
    Write(memory,esp+4,int32_t(10));
    random_drops=false;const bool off=!RandomizeDropItem(memory,esp,.9f) && Read<int32_t>(memory,esp+4)==10;
    random_drops=true;const bool picked=RandomizeDropItem(memory,esp,.9f) && Read<int32_t>(memory,esp+4)==77;
    crystal_amount=saved_amount;random_drops=saved_random;
    const bool ok=doubled&&neutral&&off&&picked;
    LOGR("dev selftest: drops crystal amount/neutral/random off/random table=%d",ok);
    return ok;
}
}
// Aleatorio [0,max) no x87. So as checagens do drop (pelo retorno) tem o sorteio dividido pelo fator.
X86_FUNC(sub_00032D90) {
    const uint32_t ret=ReturnAddress(base,ctx.esp);
    __imp__sub_00032D90(ctx,base);
    const auto check=dev::DropCheckAt(ret);
    if(check==dev::DropCheck::None) return;
    double& top=ctx.st[ctx.top&7];
    top=dev::DropRoll(top,check==dev::DropCheck::Crystal ? dev::crystal_rate.load() : dev::drop_rate.load());
}
// Cristais de evolucao soltos pelo inimigo (sub_0008EF10).
X86_FUNC(sub_002F95E0) {
    if(ReturnAddress(base,ctx.esp)==dev::kCrystalDropReturn) AdjustCrystalAmount(base,ctx.esp);
    __imp__sub_002F95E0(ctx,base);
}
// Item da ficha do inimigo (raro ou comum).
X86_FUNC(sub_002F9A30) {
    if(dev::ItemDropReturn(ReturnAddress(base,ctx.esp))) RandomizeDropItem(base,ctx.esp,Uniform());
    __imp__sub_002F9A30(ctx,base);
}
// Cria o objeto do item no chao e devolve o ponteiro (0 se falhou): aplica o tamanho escolhido.
X86_FUNC(sub_002F92F0) {
    __imp__sub_002F92F0(ctx,base);
    const float factor=dev::ClampItemScale(dev::item_scale.load());
    if(factor!=1.0f && dev::ScaleSceneObject(ctx,base,ctx.eax,factor) && logged.fetch_add(1)<16)
        LOGR("drops: item object %08X scale %.2f",ctx.eax,factor);
}
