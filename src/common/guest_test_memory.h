#pragma once
#include "guest_memory_guard.h"

namespace dev {
// Reserva virtual esparsa isolada: exercita os mesmos enderecos da pilha real.
class GuestTestMemory {
    uint8_t* memory=nullptr;
public:
    static constexpr uint32_t stack=0xD0010000;
    GuestTestMemory() {
        memory=static_cast<uint8_t*>(VirtualAlloc(nullptr,0x100000000ull,MEM_RESERVE,PAGE_NOACCESS));
        if (memory && (!VirtualAlloc(memory,0x04000000,MEM_COMMIT,PAGE_READWRITE) ||
            !VirtualAlloc(memory+stack-0x10000,0x20000,MEM_COMMIT,PAGE_READWRITE))) {
            VirtualFree(memory,0,MEM_RELEASE);memory=nullptr;
        }
    }
    ~GuestTestMemory() { if(memory) VirtualFree(memory,0,MEM_RELEASE); }
    GuestTestMemory(const GuestTestMemory&)=delete;
    GuestTestMemory& operator=(const GuestTestMemory&)=delete;
    uint8_t* data() const { return memory; }
};
}
