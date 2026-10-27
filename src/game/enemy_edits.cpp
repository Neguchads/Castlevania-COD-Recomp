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
#include <mutex>
#include <string>
#include <unordered_map>
#include "common/drop_rules.h"

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
// Escala da primeira peca do modelo: com [ator+0x100]==1, sub_0035BA40 copia o vetor em cada peca.
// Lista em [modelo+0x11C]: contagem em +0x14, nos de 0x80 bytes a partir de [lista+0x10], escala em +0x20.
float ModelNodeScale(const uint8_t* base,uint32_t actor) {
    if(base[actor+0x100]!=1) return NAN;
    const auto model=Read<uint32_t>(base,actor+0x104);
    if(!model || !dev::ReadableGuestRange(base,model,0x140)) return NAN;
    const auto list=Read<uint32_t>(base,model+0x11C);
    if(!list || !dev::ReadableGuestRange(base,list,0x18) || Read<int32_t>(base,list+0x14)<=0) return NAN;
    const auto nodes=Read<uint32_t>(base,list+0x10);
    if(!nodes || !dev::ReadableGuestRange(base,nodes,0x80)) return NAN;
    return Read<float>(base,nodes+0x20);
}
std::atomic<int> scale_logs{0};
// Edita um ator; com hp_return, o topo do x87 e a vida maxima que sub_00060010 acabou de devolver.
dev::ActorEdit EditActor(X86Context& ctx,uint8_t* base,uint32_t actor,uint32_t type,const dev::EnemyRandomSettings& random,bool hp_return) {
    const float factor=dev::CombinedEnemyScale(dev::enemy_scale.load(),dev::EnemyScaleEdit(type));
    const auto edit=dev::EnemyActorEdit(actor,type,Read<float>(base,actor+kScale),factor,random);
    // O jogo pode devolver as pecas do modelo ao tamanho dele sem passar pela escala do ator:
    // com fator ativo, a raiz ainda tem o nosso valor e as pecas recebem de novo.
    const float total=dev::ClampEnemyScale(factor*edit.roll.scale),root=Read<float>(base,actor+kScale),node=ModelNodeScale(base,actor);
    if(!edit.step.write && total!=1.0f && std::isfinite(node) && std::fabs(node-root)>1e-4f &&
       WriteScale(ctx,base,actor,{true,root,{}}) && scale_logs.fetch_add(1)<12)
        LOGR("editor: model reset actor %08X type %u node %.2f -> %.2f",actor,type,node,root);
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
namespace {
// Itens no chao criados por sub_002F92F0: endereco -> modelo, para reconhecer o mesmo objeto depois.
std::mutex item_lock;
std::unordered_map<uint32_t,uint32_t> item_objects;
bool IsItemObject(const uint8_t* base,uint32_t object) {
    std::lock_guard guard(item_lock);
    const auto it=item_objects.find(object);
    return it!=item_objects.end() && it->second==Read<uint32_t>(base,object+0x104);
}
}
namespace dev {
void NoteItemObject(const uint8_t* base,uint32_t object) {
    if(!object || !ReadableGuestRange(base,object,kActorBytes)) return;
    std::lock_guard guard(item_lock);
    if(item_objects.size()>512) item_objects.clear();
    item_objects[object]=Read<uint32_t>(base,object+0x104);
}
// O jogo acabou de escalar um objeto (sub_0035BA40): inimigo ou item no chao recebem o fator por cima.
void GameScaleSet(X86Context& ctx,uint8_t* base,uint32_t object) {
    if(!object || !ReadableGuestRange(base,object,kActorBytes)) return;
    const float current=Read<float>(base,object+kScale);
    if(Read<uint16_t>(base,object)==2) {
        const uint32_t type=Read<uint16_t>(base,object+2);
        const float factor=CombinedEnemyScale(enemy_scale.load(),EnemyScaleEdit(type));
        const float total=EnemyGameScale(object,type,current,factor,RandomSettings());
        if(total!=1.0f && WriteScale(ctx,base,object,{true,current*total,{}}) && scale_logs.fetch_add(1)<12)
            LOGR("editor: game scale actor %08X type %u %.2f -> %.2f",object,type,current,current*total);
        return;
    }
    const float factor=ClampItemScale(item_scale.load());
    if(factor!=1.0f && IsItemObject(base,object)) ScaleSceneObject(ctx,base,object,factor);
}
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
    static std::atomic<int> logged{0},probed{0};
    if((edit.rolled || edit.step.write) && logged.fetch_add(1)<12)
        LOGR("editor: enemy type %u actor %08X scale %.2f roll scale %.2f hp %.2f elite %d",
             type,actor,Read<float>(base,actor+kScale),edit.roll.scale,edit.roll.hp,int(edit.roll.elite));
    // Pesquisa: a escala fica gravada no no do ator, mas o modelo nao muda. sub_0035BA40 so repassa
    // para as pecas do modelo ([ator+0x104], lista em +0x11C) quando [ator+0x100]==1.
    if(edit.step.write && probed.fetch_add(1)<6) {
        const auto model=Read<uint32_t>(base,actor+0x104);
        const bool model_ok=model && ReadableGuestRange(base,model,0x140);
        LOGR("editor: probe actor %08X type %u b100=%u m104=%08X f130=%08X d4=%08X p20=%08X n6E=%04X m11C=%08X m138=%08X",
             actor,type,unsigned(base[actor+0x100]),model,Read<uint32_t>(base,actor+0x130),Read<uint32_t>(base,actor+4),
             Read<uint32_t>(base,actor+0x20),unsigned(Read<uint16_t>(base,actor+0x80+0x6E)),
             model_ok ? Read<uint32_t>(base,model+0x11C) : 0u,model_ok ? Read<uint32_t>(base,model+0x138) : 0u);
    }
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
    // O jogo reescala o ator (nascimento/efeito): com 2x ativo, o fator volta por cima do valor do jogo.
    const bool saved_random=enemy_random.load();enemy_random=false;
    enemy_scale=2;Write(memory,actor+kScale,std::array<float,4>{.5f,.5f,.5f,1});
    GameScaleSet(call,memory,actor);
    enemy_random=saved_random;
    const bool game_set=scale()==std::array<float,4>{1,1,1,1};
    enemy_scale=1;EditActor(call,memory,actor,7,off,false);
    // Elite garantido: vida do retorno no x87 dobra e o tamanho vai a 2x com faixa fixa em 1x.
    EnemyRandomSettings elite;elite.enabled=true;elite.elite_chance=1;elite.elite_scale=2;elite.elite_hp=2;
    Write(memory,actor+2,uint16_t(8));Write(memory,actor+kHp,100.0f);
    call.top=0;x87_push(call,100.0);
    const auto rolled=EditActor(call,memory,actor,8,elite,true);
    const bool elite_ok=rolled.roll.elite && call.st[call.top&7]==200.0 && Read<float>(memory,actor+kHp)==200.0f &&
        scale()==std::array<float,4>{2,2,2,1};
    enemy_scale=saved;
    const bool esp_kept=call.esp==GuestTestMemory::stack;
    const bool ok=grown&&stable&&restored&&game_set&&elite_ok&&esp_kept;
    LOGR("dev selftest: enemy editor scale grow/no stacking/restore/game rescale/elite hp+size/stack=%d (game rescale=%d)",ok,int(game_set));
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
