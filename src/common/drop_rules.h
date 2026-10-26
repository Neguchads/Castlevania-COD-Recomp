#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace dev {
// Drops (sub_0008EF10): cada checagem sorteia r em [0,1) com sub_00032D90 e solta o item se chance > r.
// Dividir r pelo fator multiplica a chance; fator 0 nunca solta.
inline constexpr float kMaxDropFactor = 10.0f;
inline float ClampDropFactor(float factor) {
    return std::isfinite(factor) ? std::clamp(factor, 0.0f, kMaxDropFactor) : 1.0f;
}
inline double DropRoll(double roll, float factor) {
    factor = ClampDropFactor(factor);
    if (factor == 1.0f || !std::isfinite(roll)) return roll;
    return factor > 0 ? roll / factor : 1e30;
}
// Endereco de retorno da chamada ao aleatorio -> qual chance do drop ela decide.
enum class DropCheck { None, Item, Crystal };
inline constexpr DropCheck DropCheckAt(uint32_t return_address) {
    switch (return_address) {
        case 0x8F102:   // item raro (ficha +0x78)
        case 0x8F187:   // item comum (ficha +0x74)
        case 0x8F207:   // item 0x128 (ficha +0xA0)
        case 0x8F2DE:   // item 0x125
            return DropCheck::Item;
        case 0x8F263:   // cristal de evolucao do Innocent Devil (sub_002F95E0)
            return DropCheck::Crystal;
        default:
            return DropCheck::None;
    }
}
inline constexpr uint32_t kCrystalDropReturn = 0x8F2A3;      // sub_002F95E0(tipo, quantidade, ...)
inline constexpr bool ItemDropReturn(uint32_t address) {    // sub_002F9A30(item, ...) dos itens da ficha
    return address == 0x8F130 || address == 0x8F1B5;
}
// Quantidade de cristais (o jogo reparte em ate 4 cristais).
inline constexpr float kMaxCrystalAmount = 5.0f;
inline int32_t CrystalAmount(int32_t amount, float factor) {
    if (amount <= 0) return amount;
    factor = std::isfinite(factor) ? std::clamp(factor, 1.0f, kMaxCrystalAmount) : 1.0f;
    return int32_t(std::lround(double(amount) * factor));
}
// Itens aleatorios: tabela de drops do jogo (word por entrada, 0x17E entradas em 0x548EB8); so IDs positivos.
inline constexpr uint32_t kDropTable = 0x548EB8, kDropTableEntries = 0x17E;
inline int32_t PickDropItem(const int16_t* table, uint32_t count, float u, int32_t fallback) {
    if (!table || !count) return fallback;
    u = std::isfinite(u) ? std::clamp(u, 0.0f, 1.0f) : 0.0f;
    const uint32_t start = std::min(count - 1, uint32_t(u * float(count)));
    for (uint32_t step = 0; step < count; ++step) {
        const int16_t item = table[(start + step) % count];
        if (item > 0) return item;
    }
    return fallback;
}
// Tamanho dos itens soltos no chao (escala nativa do objeto de sub_002F92F0).
inline constexpr float kMinItemScale = .25f, kMaxItemScale = 2.0f;
inline float ClampItemScale(float factor) {
    return std::isfinite(factor) ? std::clamp(factor, kMinItemScale, kMaxItemScale) : 1.0f;
}
}
