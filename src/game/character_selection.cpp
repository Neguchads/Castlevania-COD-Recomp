#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <array>
#include <cstring>
#include <vector>

X86_EXTERN_FUNC(__imp__sub_003219A0);
X86_EXTERN_FUNC(__imp__sub_002F7030);

namespace {
uint32_t Read(const uint8_t* base, uint32_t address) {
    uint32_t value; std::memcpy(&value,base+address,4); return value;
}
}

// Chamado apenas pelo estagio Novo Jogo, depois de confirmar o nome.
X86_FUNC(sub_003219A0) {
    const bool native_new_game = dev::character_selection_ready &&
        dev::ReadableGuestRange(base,ctx.esp,4) && Read(base,ctx.esp)==0x321D79;
    if (native_new_game) {
        if (dev::new_game_phase.load()==dev::NewGamePhase::Idle) {
            dev::new_game_character.store(0);
            dev::new_game_phase.store(dev::NewGamePhase::Choosing);
            LOGR("character: new game awaiting Hector/Trevor selection");
        }
        if (dev::CharacterChoosing()) {
            // Retorno pendente da FSM: o jogo continua apresentando quadros para a UI.
            ctx.eax=0;ctx.esp+=4;return;
        }
    }
    __imp__sub_003219A0(ctx,base);
    if (native_new_game && ctx.eax!=0) dev::new_game_phase.store(dev::NewGamePhase::Idle);
}

X86_FUNC(sub_002F7030) {
    const bool selecting = dev::character_selection_ready &&
        dev::new_game_phase.load()==dev::NewGamePhase::Starting &&
        dev::ReadableGuestRange(base,ctx.esp,8) && Read(base,ctx.esp)==0x3219D4;
    __imp__sub_002F7030(ctx,base);
    if (selecting) {
        ctx.eax=dev::new_game_character.load()==2 ? 2 : 0;
        LOGR("character: native new game mode=%u (%s)",ctx.eax,ctx.eax==2 ? "Trevor" : "Hector");
    }
}

namespace dev {
bool ValidateCharacterSelection() {
    if (!kernel::g_base) return false;
    constexpr std::array<uint8_t,12> classifier{0x53,0x8B,0x5C,0x24,0x08,0x55,0x56,0x57,0xBF,0x9C,0x8D,0x54};
    constexpr std::array<uint8_t,8> name{0x40,0x54,0x52,0x45,0x56,0x4F,0x52,0};
    constexpr std::array<uint8_t,8> init{0x56,0x33,0xF6,0xE8,0x98,0x27,0xD2,0xFF};
    const bool valid=std::memcmp(kernel::g_base+0x2F7030,classifier.data(),classifier.size())==0 &&
        std::memcmp(kernel::g_base+0x43AED0,name.data(),name.size())==0 &&
        std::memcmp(kernel::g_base+0x3219A0,init.data(),init.size())==0;
    character_selection_ready.store(valid);
    return valid;
}

bool VerifyCharacterSelection() {
    if (!character_selection_ready) return false;
    GuestTestMemory arena;
    if (!arena.data()) return false;
    std::memcpy(arena.data()+0x548D9C,kernel::g_base+0x548D9C,24);
    std::memcpy(arena.data()+0x43AED0,kernel::g_base+0x43AED0,16);
    auto write=[&](uint32_t address,uint32_t value){std::memcpy(arena.data()+address,&value,4);};
    write(0x548C18,0x200000);
    const auto saved_phase=new_game_phase.load();
    const int saved_character=new_game_character.load();
    auto classify=[&](const char* name,NewGamePhase phase,int character,uint32_t caller,uint32_t flags){
        std::memcpy(arena.data()+0x210000,name,std::strlen(name)+1);
        write(0x200008,flags);write(GuestTestMemory::stack,caller);write(GuestTestMemory::stack+4,0x210000);
        X86Context ctx{};ctx.esp=GuestTestMemory::stack;
        new_game_phase.store(phase);new_game_character.store(character);
        sub_002F7030(ctx,arena.data());
        return ctx.esp==GuestTestMemory::stack+4 ? ctx.eax : 0xFFFFFFFFu;
    };
    const bool original=classify("@TREVOR",NewGamePhase::Idle,0,0x3219D4,0)==0;
    const bool trevor=classify("PLAYER",NewGamePhase::Starting,2,0x3219D4,0)==2;
    const bool hector=classify("@TREVOR",NewGamePhase::Starting,0,0x3219D4,8)==0;
    const bool other=classify("@CRAZY",NewGamePhase::Starting,2,0x1234,16)==1;
    new_game_phase.store(NewGamePhase::Idle);
    X86Context pending{};pending.esp=GuestTestMemory::stack;write(pending.esp,0x321D79);
    sub_003219A0(pending,arena.data());
    const bool queued=CharacterChoosing() && pending.eax==0 && pending.esp==GuestTestMemory::stack+4;
    new_game_phase.store(saved_phase);new_game_character.store(saved_character);
    const bool untouched=Read(arena.data(),0xB23700)==0;
    LOGR("dev selftest: native character selection original=%d Trevor=%d Hector=%d unrelated=%d pending=%d no-save-write=%d",
        original,trevor,hector,other,queued,untouched);
    return original && trevor && hector && other && queued && untouched;
}
}
