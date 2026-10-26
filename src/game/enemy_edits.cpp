#include "game/enemy_edits.h"
#include "common/dev_runtime.h"
#include "common/enemy_editor.h"
#include "common/guest_memory_guard.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <array>
#include <atomic>
#include <cstring>
#include <string>

X86_EXTERN_FUNC(__imp__sub_0035BA40);
X86_EXTERN_FUNC(__imp__sub_00060010);
namespace {
template<class T> T Read(const uint8_t* base,uint32_t address) {T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
template<class T> void Write(uint8_t* base,uint32_t address,T value) {std::memcpy(base+address,&value,sizeof(T));}
// Ator (objeto de cena de sub_0035A480): +0 tipo de personagem (2 = inimigo), +2 tipo do inimigo,
// +0x80 no da cena (escala em +0xA0), +0x128 ficha do painel quando [+0x130] tem 0x10 e 0x20 (nome em +0x30).
constexpr uint32_t kActorBytes=0x140,kScale=0xA0;
std::string EnemyName(const uint8_t* base,uint32_t actor) {
    if((Read<uint32_t>(base,actor+0x130)&0x30)!=0x30) return {};
    const auto info=Read<uint32_t>(base,actor+0x128);
    if(!info || !dev::ReadableGuestRange(base,info,0x34)) return {};
    const auto text=Read<uint32_t>(base,info+0x30);
    if(!text || !dev::ReadableGuestRange(base,text,48)) return {};
    std::string name;
    for(uint32_t i=0;i<48;++i) {
        const char c=char(base[text+i]);
        if(!c) break;
        if(c<32 || c>126) return {};
        name+=c;
    }
    return name;
}
// Escala nativa (sub_0035BA40 copia o vetor no no e recalcula, inclusive partes em [obj+0x104]).
// Argumentos e vetor ficam abaixo da pilha atual, livre depois do retorno do original.
bool ApplyScale(X86Context& ctx,uint8_t* base,uint32_t actor,float factor) {
    const auto current=Read<std::array<float,4>>(base,actor+kScale);
    const auto step=dev::EnemyScaleStep(actor,current[0],factor);
    if(!step.write) return false;
    const uint32_t stack=ctx.esp-256;
    if(ctx.esp<512 || !dev::ReadableGuestRange(base,stack,64)) return false;
    auto scale=current;
    const float ratio=step.value/current[0];
    for(int axis=0;axis<3;++axis) scale[axis]*=ratio;
    X86Context call=ctx;call.esp=stack;
    Write(base,stack,uint32_t(0));Write(base,stack+4,actor);Write(base,stack+8,stack+16);
    Write(base,stack+16,scale);
    __imp__sub_0035BA40(call,base);
    return true;
}
}
namespace dev {
void ApplyEnemyEdits(X86Context& ctx,uint8_t* base,uint32_t actor) {
    if(!actor || !ReadableGuestRange(base,actor,kActorBytes) || Read<uint16_t>(base,actor)!=2) return;
    const uint32_t type=Read<uint16_t>(base,actor+2);
    NoteEnemy(type,EnemyName(base,actor));
    const float factor=EnemyScaleEdit(type);
    if(ApplyScale(ctx,base,actor,factor)) {
        static std::atomic<int> logged{0};
        if(logged.fetch_add(1)<8) LOGR("editor: enemy type %u actor %08X scale %.2f",type,actor,Read<float>(base,actor+kScale));
    }
}
bool VerifyEnemyEditorHooks() {
    GuestTestMemory arena;if(!arena.data())return false;
    auto* memory=arena.data();constexpr uint32_t actor=0x210000;
    Write(memory,actor,uint16_t(2));Write(memory,actor+2,uint16_t(7));
    Write(memory,actor+kScale,std::array<float,4>{1,1,1,1});
    X86Context call{};call.esp=GuestTestMemory::stack;
    auto scale=[&]{return Read<std::array<float,4>>(memory,actor+kScale);};
    const bool grown=ApplyScale(call,memory,actor,2) && scale()==std::array<float,4>{2,2,2,1};
    const bool stable=!ApplyScale(call,memory,actor,2) && scale()==std::array<float,4>{2,2,2,1};
    const bool restored=ApplyScale(call,memory,actor,1) && scale()==std::array<float,4>{1,1,1,1};
    const bool esp_kept=call.esp==GuestTestMemory::stack;
    const bool ok=grown&&stable&&restored&&esp_kept;
    LOGR("dev selftest: enemy editor scale grow/no stacking/restore/stack=%d",ok);
    return ok;
}
}
// Vida maxima por ator (cdecl, ator em [esp+4]): chamada ao nascer e nas checagens de vida da IA;
// aproveita para aplicar o editor. O retorno no x87 nao e tocado.
X86_FUNC(sub_00060010) {
    uint32_t actor=0;
    if(dev::ReadableGuestRange(base,ctx.esp,8)) std::memcpy(&actor,base+ctx.esp+4,4);
    __imp__sub_00060010(ctx,base);
    dev::ApplyEnemyEdits(ctx,base,actor);
}
