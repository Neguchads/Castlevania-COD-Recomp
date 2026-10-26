#include "game/enemy_edits.h"
#include "common/dev_runtime.h"
#include "common/enemy_editor.h"
#include "common/guest_memory_guard.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <string>

X86_EXTERN_FUNC(__imp__sub_0035BA40);
X86_EXTERN_FUNC(__imp__sub_00060010);
namespace {
template<class T> T Read(const uint8_t* base,uint32_t address) {T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
template<class T> void Write(uint8_t* base,uint32_t address,T value) {std::memcpy(base+address,&value,sizeof(T));}
// Ator (objeto de cena de sub_0035A480): +0 tipo de personagem (2 = inimigo), +2 tipo do inimigo,
// +0x80 no da cena (escala em +0xA0), +0x128 ficha do painel quando [+0x130] tem 0x10 e 0x20 (nome em +0x30).
constexpr uint32_t kActorBytes=0x140,kHp=0x1C,kScale=0xA0;
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
dev::EnemyRandomSettings RandomSettings() {
    dev::EnemyRandomSettings random;
    random.enabled=dev::enemy_random.load();
    random.scale_min=dev::enemy_random_scale_min.load();random.scale_max=dev::enemy_random_scale_max.load();
    random.hp_min=dev::enemy_random_hp_min.load();random.hp_max=dev::enemy_random_hp_max.load();
    random.elite_chance=dev::enemy_elite_chance.load()/100.0f;
    random.elite_scale=dev::enemy_elite_scale.load();random.elite_hp=dev::enemy_elite_hp.load();
    return random;
}
// Escala nativa (sub_0035BA40 copia o vetor no no e recalcula, inclusive partes em [obj+0x104]).
// Argumentos e vetor ficam abaixo da pilha atual, livre depois do retorno do original.
bool WriteScale(X86Context& ctx,uint8_t* base,uint32_t actor,const dev::ScaleStep& step) {
    if(!step.write) return false;
    const uint32_t stack=ctx.esp-256;
    if(ctx.esp<512 || !dev::ReadableGuestRange(base,stack,64)) return false;
    auto scale=Read<std::array<float,4>>(base,actor+kScale);
    const float ratio=step.value/scale[0];
    for(int axis=0;axis<3;++axis) scale[axis]*=ratio;
    X86Context call=ctx;call.esp=stack;
    Write(base,stack,uint32_t(0));Write(base,stack+4,actor);Write(base,stack+8,stack+16);
    Write(base,stack+16,scale);
    __imp__sub_0035BA40(call,base);
    return true;
}
// Edita um ator; com hp_return, o topo do x87 e a vida maxima que sub_00060010 acabou de devolver.
dev::ActorEdit EditActor(X86Context& ctx,uint8_t* base,uint32_t actor,uint32_t type,const dev::EnemyRandomSettings& random,bool hp_return) {
    const float factor=dev::CombinedEnemyScale(dev::enemy_scale.load(),dev::EnemyScaleEdit(type));
    const auto edit=dev::EnemyActorEdit(actor,type,Read<float>(base,actor+kScale),factor,random);
    // A barra do inimigo divide [ator+0x1C] por este retorno; vida cheia acompanha a maxima sorteada.
    if(hp_return && edit.roll.hp!=1.0f) {
        double& top=ctx.st[ctx.top&7];
        const double raw=top;
        top=dev::EnemyMaxHp(raw,edit.roll.hp);
        const float hp=Read<float>(base,actor+kHp),lifted=dev::RolledCurrentHp(hp,raw,top);
        if(lifted!=hp) Write(base,actor+kHp,lifted);
    }
    WriteScale(ctx,base,actor,edit.step);
    return edit;
}
}
namespace dev {
bool ScaleSceneObject(X86Context& ctx,uint8_t* base,uint32_t object,float factor) {
    if(!object || factor==1.0f || !std::isfinite(factor) || factor<=0 || !ReadableGuestRange(base,object,kActorBytes)) return false;
    const float current=Read<float>(base,object+kScale);
    if(!std::isfinite(current) || current<=0) return false;
    return WriteScale(ctx,base,object,{true,current*factor,{}});
}
void ApplyEnemyEdits(X86Context& ctx,uint8_t* base,uint32_t actor,bool hp_return) {
    if(!actor || !ReadableGuestRange(base,actor,kActorBytes) || Read<uint16_t>(base,actor)!=2) return;
    const uint32_t type=Read<uint16_t>(base,actor+2);
    NoteEnemy(type,EnemyName(base,actor));
    const auto edit=EditActor(ctx,base,actor,type,RandomSettings(),hp_return);
    static std::atomic<int> logged{0};
    if((edit.rolled || edit.step.write) && logged.fetch_add(1)<12)
        LOGR("editor: enemy type %u actor %08X scale %.2f roll scale %.2f hp %.2f elite %d",
             type,actor,Read<float>(base,actor+kScale),edit.roll.scale,edit.roll.hp,int(edit.roll.elite));
}
bool VerifyEnemyEditorHooks() {
    GuestTestMemory arena;if(!arena.data())return false;
    auto* memory=arena.data();constexpr uint32_t actor=0x210000;
    Write(memory,actor,uint16_t(2));Write(memory,actor+2,uint16_t(7));
    Write(memory,actor+kScale,std::array<float,4>{1,1,1,1});
    X86Context call{};call.esp=GuestTestMemory::stack;
    auto scale=[&]{return Read<std::array<float,4>>(memory,actor+kScale);};
    const float saved=enemy_scale.load();
    EnemyRandomSettings off;
    enemy_scale=2;const bool grown=EditActor(call,memory,actor,7,off,false).step.write && scale()==std::array<float,4>{2,2,2,1};
    const bool stable=!EditActor(call,memory,actor,7,off,false).step.write && scale()==std::array<float,4>{2,2,2,1};
    enemy_scale=1;const bool restored=EditActor(call,memory,actor,7,off,false).step.write && scale()==std::array<float,4>{1,1,1,1};
    // Elite garantido: vida do retorno no x87 dobra e o tamanho vai a 2x com faixa fixa em 1x.
    EnemyRandomSettings elite;elite.enabled=true;elite.elite_chance=1;elite.elite_scale=2;elite.elite_hp=2;
    Write(memory,actor+2,uint16_t(8));Write(memory,actor+kHp,100.0f);
    call.top=0;x87_push(call,100.0);
    const auto rolled=EditActor(call,memory,actor,8,elite,true);
    const bool elite_ok=rolled.roll.elite && call.st[call.top&7]==200.0 && Read<float>(memory,actor+kHp)==200.0f &&
        scale()==std::array<float,4>{2,2,2,1};
    enemy_scale=saved;
    const bool esp_kept=call.esp==GuestTestMemory::stack;
    const bool ok=grown&&stable&&restored&&elite_ok&&esp_kept;
    LOGR("dev selftest: enemy editor scale grow/no stacking/restore/elite hp+size/stack=%d",ok);
    return ok;
}
}
// Vida maxima por ator (cdecl, ator em [esp+4]): chamada ao nascer e nas checagens de vida da IA.
// Aplica tamanho e o sorteio de vida do ator no retorno (topo do x87), sempre o mesmo para o mesmo ator.
X86_FUNC(sub_00060010) {
    uint32_t actor=0;
    if(dev::ReadableGuestRange(base,ctx.esp,8)) std::memcpy(&actor,base+ctx.esp+4,4);
    __imp__sub_00060010(ctx,base);
    dev::ApplyEnemyEdits(ctx,base,actor,true);
}
