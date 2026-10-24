#include "../src/common/guest_test_memory.h"
#include <cstdio>
int main() {
    dev::GuestTestMemory arena;
    if (!arena.data()) return 1;
    if (!dev::ReadableGuestRange(arena.data(),dev::GuestTestMemory::stack,20)) return 2;
    if (!dev::ReadableGuestRange(arena.data(),0x100000,20)) return 3;
    if (dev::ReadableGuestRange(arena.data(),0xFFFFFFFC,8)) return 4;
    if (dev::ReadableGuestRange(arena.data(),0xD1000000,4)) return 5;
    if (dev::ReadableGuestRange(arena.data(),0x03FFFFFC,8)) return 6;
    DWORD previous=0;
    if (!VirtualProtect(arena.data()+dev::GuestTestMemory::stack,0x1000,PAGE_READWRITE|PAGE_GUARD,&previous)) return 7;
    if (dev::ReadableGuestRange(arena.data(),dev::GuestTestMemory::stack,4)) return 8;
    std::puts("Guest memory: low RAM, high stack, overflow, uncommitted page and guard passed");
}
