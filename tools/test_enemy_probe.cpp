#include "../src/common/enemy_probe_rules.h"
#include <array>
#include <cstdio>

int main() {
    using namespace dev;
    std::array<uint8_t, 32> before{}, after{};
    auto put32 = [](auto& a, size_t o, int32_t v) { std::memcpy(a.data() + o, &v, 4); };
    auto put16 = [](auto& a, size_t o, int16_t v) { std::memcpy(a.data() + o, &v, 2); };
    // Vida int32 em +4 cai de 120 para 95; contador em +8 sobe; nada mais muda.
    put32(before, 4, 120); put32(after, 4, 95);
    put32(before, 8, 10); put32(after, 8, 11);
    auto drops = DroppedFields(before, after);
    if (drops.size() != 1 || drops[0].offset != 4 || drops[0].bits != 32 || drops[0].before != 120 || drops[0].after != 95) return 1;
    // Vida int16 em +14 (metade alta de um int32): aparece como 16 bits.
    put16(before, 14, 300); put16(after, 14, 0);
    drops = DroppedFields(before, after);
    if (drops.size() != 2 || drops[1].offset != 14 || drops[1].bits != 16 || drops[1].after != 0) return 2;
    // Antes <= 0 nao conta; queda int32 grande demais nao vira int32 (so a metade alta, 4 -> 1, como int16).
    std::array<uint8_t, 32> b2{}, a2{};
    put32(b2, 0, 0); put32(a2, 0, -5);
    put32(b2, 4, 300000); put32(a2, 4, 100000);
    drops = DroppedFields(b2, a2);
    if (drops.size() != 1 || drops[0].offset != 6 || drops[0].bits != 16 || drops[0].before != 4 || drops[0].after != 1) return 3;
    if (!DroppedFields(std::span<const uint8_t>(before.data(), 3), after).empty()) return 4;
    // Limite de resultados.
    std::array<uint8_t, 32> b3{}, a3{};
    for (size_t o = 0; o < 32; o += 4) { put32(b3, o, 50); put32(a3, o, 40); }
    if (DroppedFields(b3, a3, 3).size() != 3) return 5;
    std::puts("Enemy probe: int32 and int16 drops, ignored rises, bounds and limit passed");
}
