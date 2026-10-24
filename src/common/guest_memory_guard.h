#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdint>

namespace dev {
// Pilhas Xbox usam enderecos virtuais altos; RAM fisica nao e limite de ESP.
inline bool ReadableGuestRange(const uint8_t* base, uint32_t address, uint32_t bytes) {
    const uint64_t end=uint64_t(address)+bytes;
    if (!base || !bytes || end>0x100000000ull) return false;
    auto cursor=reinterpret_cast<uintptr_t>(base)+address;
    const auto finish=reinterpret_cast<uintptr_t>(base)+end;
    while (cursor<finish) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<const void*>(cursor),&info,sizeof(info)) ||
            info.State!=MEM_COMMIT || (info.Protect & (PAGE_GUARD|PAGE_NOACCESS))) return false;
        const auto next=reinterpret_cast<uintptr_t>(info.BaseAddress)+info.RegionSize;
        if (next<=cursor) return false;
        cursor=next;
    }
    return true;
}
}
