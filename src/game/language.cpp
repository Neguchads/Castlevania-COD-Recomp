#include "xdk/hle.h"
#include "common/dev_runtime.h"
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
// Primeira fatia ASCII: a fonte nativa ainda precisa de auditoria dos acentos.
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
int TextLanguage() {
    if(ptbr_enabled)return 2;
    if(kernel::g_base && ReadableGuestRange(kernel::g_base,0xB235E0,4))return Read<uint32_t>(kernel::g_base,0xB235E0)==1 ? 1 : 0;
    return 1;
}
void SelectTextLanguage(int language) {
    ptbr_enabled=language==2;
    if(kernel::g_base&&ReadableGuestRange(kernel::g_base,0xB235E0,4))Write(kernel::g_base,0xB235E0,language==0 ? 0 : 1);
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
        if(auto address=Store(0xFFFFFFFE,"Portugues (Brasil)"))ctx.eax=address;
    } else if(dev::ptbr_enabled) {
        if(auto it=translations.find(id);it!=translations.end())if(auto address=Store(id,it->second))ctx.eax=address;
    }
}
X86_FUNC(sub_0030B4F0) {
    __imp__sub_0030B4F0(ctx,base);candidate=dev::ptbr_enabled ? 2 : Read<uint32_t>(base,0xB25364)==1 ? 1 : 0;
}
X86_FUNC(sub_000423F0) {
    const bool caption=dev::ReadableGuestRange(base,ctx.esp,4)&&Read<uint32_t>(base,ctx.esp)==0x30B537;
    __imp__sub_000423F0(ctx,base);if(caption)caption_input=ctx.eax;
}
X86_FUNC(sub_0030B520) {
    const auto voice=Read<uint32_t>(base,0xB25364);
    caption_input=0;__imp__sub_0030B520(ctx,base);
    if(caption_input&12) {
        candidate=(candidate+((caption_input&8) ? 1 : 2))%3;
        Write(base,0xB25364,candidate==2 ? voice : candidate==0 ? 0 : 1);
        if(candidate==2)Write(base,0xB25360,uint32_t(1));
        // Aplicacao imediata da preferencia host; indices do jogo permanecem 0/1.
        dev::ptbr_enabled=candidate==2;dev::language_dirty=true;
        LOGR("language: native caption selection=%d (0 Japanese,1 English,2 pt-BR)",candidate);
    }
}
X86_FUNC(sub_002F62E0) {
    const bool caption=dev::ReadableGuestRange(base,ctx.esp,4)&&Read<uint32_t>(base,ctx.esp)==0x30B827;
    __imp__sub_002F62E0(ctx,base);
    if(caption){dev::ptbr_enabled=candidate==2;dev::language_dirty=true;}
}
