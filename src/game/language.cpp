#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/translation_pack_rules.h"
#include "common/guest_memory_guard.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include "common/ptbr_strings.h"
#include <map>
#include <mutex>
#include <cstring>

X86_EXTERN_FUNC(__imp__sub_003681B0);
X86_FUNC(sub_003681B0);
X86_EXTERN_FUNC(__imp__sub_0030B4F0);
X86_EXTERN_FUNC(__imp__sub_0030B520);
X86_EXTERN_FUNC(__imp__sub_000423F0);
X86_EXTERN_FUNC(__imp__sub_002F62E0);
namespace {
template<class T> T Read(uint8_t* base,uint32_t address){T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
void Write(uint8_t* base,uint32_t address,uint32_t value){std::memcpy(base+address,&value,4);}
// Catalogo em UTF-8, como a tabela PAL PT-BR; o atlas da fonte segue Windows-1252.
using cod_ptbr::translations;
std::mutex strings_lock;
std::map<uint32_t,uint32_t> strings;
uint32_t storage=0,used=0;
uint32_t Store(uint32_t id,const char* text) {
    std::lock_guard guard(strings_lock);
    if(auto it=strings.find(id);it!=strings.end())return it->second;
    if(!storage){uint32_t size=1048576;if(mem::AllocateVirtual(storage,size,mem::kMemReserve|mem::kMemCommit,4)!=0)return 0;}
    const size_t length=std::strlen(text)+1;if(used+length>1048576)return 0;
    const uint32_t address=storage+used;used+=uint32_t(length);
    std::memcpy(kernel::g_base+address,text,length);strings.emplace(id,address);return address;
}
thread_local uint32_t caption_input=0;
int candidate=1;
}
namespace dev {
void SelectTextLanguage(int language) {
    const bool translated=language==2;
    if(ptbr_enabled.exchange(translated)!=translated) {
        texture_generation.fetch_add(1);
        // Imagens e vozes ja carregadas ficam em cache ate o jogo reabrir os arquivos.
        NotifyToast(translated ? "Português (Brasil) ativo. Reinicie o jogo para carregar todas as imagens e vozes."
                               : "Idioma original ativo. Reinicie o jogo para restaurar todas as imagens e vozes.");
    }
    // Ingles e PT-BR (que traduz a base inglesa) usam texto (B235E0) e voz (B235E4, faixa dublada) em ingles;
    // o seletor do menu dev tambem tira o jogo do japones.
    if(language>=1&&kernel::g_base&&ReadableGuestRange(kernel::g_base,0xB235E0,8)) {
        Write(kernel::g_base,0xB235E0,1);Write(kernel::g_base,0xB235E4,1);
    }
    language_dirty=true;
}
bool VerifyLanguageHooks() {
    GuestTestMemory arena;if(!arena.data())return false;
    auto* base=arena.data();
    Write(base,0xC7B710,0x210000);Write(base,0x210000+16+1409*4,0x220000);
    std::memcpy(base+0x220000,"Map",4);
    const bool saved=ptbr_enabled.load();
    X86Context call{};auto text=[&](bool enabled) {
        ptbr_enabled=enabled;call.esp=GuestTestMemory::stack;Write(base,call.esp,0x12345);Write(base,call.esp+4,1409);
        sub_003681B0(call,base);return call.eax;
    };
    const auto english=text(false),portuguese=text(true);
    bool ok=english==0x220000 && portuguese!=english && ReadableGuestRange(kernel::g_base,portuguese,5) &&
        std::strcmp(reinterpret_cast<char*>(kernel::g_base+portuguese),"Mapa")==0;
    for(const auto& [id,translation]:translations) {
        Write(base,0x210000+16+id*4,0x220000);
        call.esp=GuestTestMemory::stack;Write(base,call.esp,0x12345);Write(base,call.esp+4,id);
        sub_003681B0(call,base);
        ok=ok&&ReadableGuestRange(kernel::g_base,call.eax,uint32_t(std::strlen(translation)+1))&&
            std::strcmp(reinterpret_cast<char*>(kernel::g_base+call.eax),translation)==0;
    }
    ptbr_enabled=saved;
    LOGR("dev selftest: PTBR text hook=%d translations=%u preserves-original=%d",ok,unsigned(translations.size()),english==0x220000);
    LOGR("dev selftest: PTBR complete catalog allocation/token-safe lookup=%d",ok);
    return ok;
}
}
X86_FUNC(sub_003681B0) {
    uint32_t id=0,caller=0;
    if(dev::ReadableGuestRange(base,ctx.esp,8)){id=Read<uint32_t>(base,ctx.esp+4);caller=Read<uint32_t>(base,ctx.esp);}
    __imp__sub_003681B0(ctx,base);
    if(caller==0x30B750 && candidate==2) {
        if(auto address=Store(0xFFFFFFFE,"Português (Brasil)"))ctx.eax=address;
    } else if(dev::ptbr_enabled) {
        if(auto it=translations.find(id);it!=translations.end())if(auto address=Store(id,it->second))ctx.eax=address;
    }
}
// Opcoes > Idioma: B25360/B25364 sao a copia editada de B235E0 (texto) e B235E4 (voz).
X86_FUNC(sub_0030B4F0) {
    __imp__sub_0030B4F0(ctx,base);candidate=dev::AppliedLanguage(dev::ptbr_enabled,Read<uint32_t>(base,0xB25364));
}
X86_FUNC(sub_000423F0) {
    const bool caption=dev::ReadableGuestRange(base,ctx.esp,4)&&Read<uint32_t>(base,ctx.esp)==0x30B537;
    __imp__sub_000423F0(ctx,base);if(caption)caption_input=ctx.eax;
}
X86_FUNC(sub_0030B520) {
    const auto voice=Read<uint32_t>(base,0xB25364);
    // Valor fora de 0/1 so durante a chamada: o jogo ve a troca e abre a confirmacao.
    const bool pending=dev::PortugueseChangePending(candidate,dev::ptbr_enabled);
    if(pending)Write(base,0xB25364,uint32_t(2));
    caption_input=0;__imp__sub_0030B520(ctx,base);
    if(pending) {
        Write(base,0xB25364,voice);
        if((caption_input&0x230)&&!(caption_input&12))Write(base,0x012FC060,voice==1 ? 1u : 2u);
    }
    if(caption_input&12) {
        candidate=dev::NextLanguage(candidate,caption_input&8);
        Write(base,0xB25364,dev::NativeVoice(candidate));
        if(candidate==2)Write(base,0xB25360,uint32_t(1));
        // Aplica so no "Sim" da confirmacao (sub_002F62E0), como ingles/japones.
        LOGR("language: native selection candidate=%d (0 Japanese,1 English,2 pt-BR)",candidate);
    }
}
X86_FUNC(sub_002F62E0) {
    const bool caption=dev::ReadableGuestRange(base,ctx.esp,4)&&Read<uint32_t>(base,ctx.esp)==0x30B827;
    __imp__sub_002F62E0(ctx,base);
    if(caption) {
        dev::SelectTextLanguage(candidate);
        LOGR("language: confirmed=%d pt-BR pack=%s",candidate,dev::TranslationPackAvailable() ? "installed" : "missing");
    }
}
// Lista de Opcoes: com partida ativa (variavel 0x4007) o jogo esconde a linha Idioma
// (6 linhas em vez de 7, sub_003159C0/3159D0 e desenho em 0x316222). Sempre mostrar.
X86_FUNC(sub_003159A0) {
    ctx.eax=1;ctx.esp+=4;
}
