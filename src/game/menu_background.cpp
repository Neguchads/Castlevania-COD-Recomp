#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/guest_memory_guard.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <cstring>
X86_EXTERN_FUNC(__imp__sub_00359120);
X86_EXTERN_FUNC(__imp__sub_00359140);
namespace {
uint32_t Read(uint8_t* base,uint32_t address){uint32_t value;std::memcpy(&value,base+address,4);return value;}
void Write(uint8_t* base,uint32_t address,uint32_t value){std::memcpy(base+address,&value,4);}
template<class Original> void Mask(X86Context& ctx,uint8_t* base,Original original,uint32_t caller,uint32_t field) {
    if(!dev::game_menu_live || !dev::ReadableGuestRange(base,ctx.esp,8) || Read(base,ctx.esp)!=caller ||
       !dev::ReadableGuestRange(base,0xC31BE8,4)){original(ctx,base);return;}
    const uint32_t depth=Read(base,0xC31BE8);
    if(!depth || depth>4){original(ctx,base);return;}
    const uint32_t saved=0xC31A50+0x158+(depth-1)*16+field;
    if(!dev::ReadableGuestRange(base,saved,4)){original(ctx,base);return;}
    const auto stack=ctx.esp,value=Read(base,stack+4);
    Write(base,stack+4,value|Read(base,saved));original(ctx,base);Write(base,stack+4,value);
}
}
X86_FUNC(sub_00359120) {Mask(ctx,base,__imp__sub_00359120,0x30BEF8,0);}
X86_FUNC(sub_00359140) {Mask(ctx,base,__imp__sub_00359140,0x30BD52,4);}
namespace dev {
bool VerifyMenuBackground() {
    GuestTestMemory arena;if(!arena.data())return false;auto* base=arena.data();
    const bool saved=game_menu_live.load();game_menu_live=true;
    Write(base,0xC31BE8,1);Write(base,0xC31BA8,0xFFF);Write(base,0xC31BAC,0xF00);
    Write(base,0xC31BA0,0x10); // Grupo de criacao nunca e alterado.
    X86Context call{};call.esp=GuestTestMemory::stack;Write(base,call.esp,0x30BEF8);Write(base,call.esp+4,0x10);
    sub_00359120(call,base);
    bool ok=Read(base,0xC31B98)==0xFFF && Read(base,GuestTestMemory::stack+4)==0x10 && Read(base,0xC31BA0)==0x10;
    call.esp=GuestTestMemory::stack;Write(base,call.esp,0x30BD52);sub_00359140(call,base);
    ok=ok && Read(base,0xC31B9C)==0xF10;
    call.esp=GuestTestMemory::stack;Write(base,call.esp,0x12345);sub_00359120(call,base);
    ok=ok && Read(base,0xC31B98)==0x10 && Read(base,0xC31BA0)==0x10;
    game_menu_live=saved;LOGR("dev selftest: native menu live masks/restore/caller-guard=%d",ok);return ok;
}
}
