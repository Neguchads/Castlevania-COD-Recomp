#pragma once
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace dev {
// Sonda da vida dos inimigos: compara a memoria do alvo entre dois quadros e devolve os inteiros que cairam.
// Golpe tira vida; contadores de animacao sobem ou oscilam, entao so queda com antes > 0 interessa.
struct FieldDrop { uint32_t offset; int bits; int32_t before, after; };
inline constexpr int32_t kMaxProbeDrop = 100000;
inline std::vector<FieldDrop> DroppedFields(std::span<const uint8_t> before, std::span<const uint8_t> after, size_t limit = 8) {
    std::vector<FieldDrop> drops;
    const size_t size = before.size() < after.size() ? before.size() : after.size();
    auto check = [&](uint32_t offset, int bits, int32_t a, int32_t b) {
        if (drops.size() < limit && a > 0 && b < a && a - b <= kMaxProbeDrop) drops.push_back({offset, bits, a, b});
    };
    for (uint32_t offset = 0; offset + 4 <= size; offset += 4) {
        int32_t a, b;
        std::memcpy(&a, before.data() + offset, 4); std::memcpy(&b, after.data() + offset, 4);
        if (a == b) continue;
        // int32 que caiu cobre as duas metades; senao confere cada int16 (vida curta em estrutura compacta).
        if (a > 0 && b < a && a - b <= kMaxProbeDrop) { check(offset, 32, a, b); continue; }
        for (uint32_t half = 0; half < 4; half += 2) {
            int16_t x, y;
            std::memcpy(&x, before.data() + offset + half, 2); std::memcpy(&y, after.data() + offset + half, 2);
            if (x != y) check(offset + half, 16, x, y);
        }
    }
    return drops;
}
}
