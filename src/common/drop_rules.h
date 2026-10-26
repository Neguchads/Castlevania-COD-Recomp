#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

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

// Diagnostico do cristal (sub_0008EF10). O jogo so sorteia o cristal se: modo normal (sub_002F7320 == 0),
// Innocent Devil ativo (sub_00144F00 != 0), atributo 73 do Hector (id 0) != 0, indice do Devil
// (sub_001441F0) >= 0 e bit 0 do atributo 29 do Devil desligado. A tela de status do Devil le o mesmo
// atributo 73 antes de mostrar a evolucao. Valor ausente = o jogo saiu antes de ler.
inline constexpr uint32_t kAttrHectorCrystal = 0x49, kAttrDevilCrystal = 0x1D;
enum class CrystalProbe { None, Mode, Devil, DevilIndex, Hector73, Devil29, Chance };
inline constexpr CrystalProbe CrystalProbeAt(uint32_t return_address) {
    switch (return_address) {
        case 0x8EF50: return CrystalProbe::DevilIndex;  // sub_001441F0(0)
        case 0x8EFBD: return CrystalProbe::Mode;        // sub_002F7320()
        case 0x8EFC7: return CrystalProbe::Devil;       // sub_00144F00(0)
        case 0x8EFD7: return CrystalProbe::Hector73;    // sub_00336660(0, 0x49)
        case 0x8EFEA: return CrystalProbe::Devil29;     // sub_00336660(devil, 0x1D)
        case 0x8F255: return CrystalProbe::Chance;      // sub_000908D0(tipo): chance base no x87
        default: return CrystalProbe::None;
    }
}
struct CrystalGate {
    std::optional<int32_t> mode, devil_index, hector73, devil29;
    std::optional<uint32_t> devil;
    std::optional<double> chance, roll;
    bool spawned = false;
};
inline void RecordCrystalProbe(CrystalGate& gate, CrystalProbe probe, uint32_t value) {
    switch (probe) {
        case CrystalProbe::Mode: gate.mode = int32_t(value); break;
        case CrystalProbe::Devil: gate.devil = value; break;
        case CrystalProbe::DevilIndex: gate.devil_index = int32_t(value); break;
        case CrystalProbe::Hector73: gate.hector73 = int32_t(value); break;
        case CrystalProbe::Devil29: gate.devil29 = int32_t(value); break;
        default: break;
    }
}
// Primeira condicao que barrou o cristal, na ordem em que o jogo testa.
enum class CrystalBlock { Allowed, NotReached, Mode, NoDevil, Hector73, Devil29 };
inline constexpr int kCrystalBlocks = 6;
inline CrystalBlock CrystalBlockOf(const CrystalGate& g) {
    if (!g.mode) return CrystalBlock::NotReached;
    if (*g.mode != 0) return CrystalBlock::Mode;
    if (!g.devil) return CrystalBlock::NotReached;
    if (*g.devil == 0) return CrystalBlock::NoDevil;
    if (!g.hector73) return CrystalBlock::NotReached;
    if (*g.hector73 == 0) return CrystalBlock::Hector73;
    if (!g.devil_index || *g.devil_index < 0) return CrystalBlock::NoDevil;
    if (!g.devil29) return CrystalBlock::NotReached;
    if (*g.devil29 & 1) return CrystalBlock::Devil29;
    return CrystalBlock::Allowed;
}
inline const char* CrystalBlockName(CrystalBlock block) {
    switch (block) {
        case CrystalBlock::Allowed: return "liberado";
        case CrystalBlock::NotReached: return "nao_chegou";
        case CrystalBlock::Mode: return "modo_especial";
        case CrystalBlock::NoDevil: return "sem_devil";
        case CrystalBlock::Hector73: return "hector73_zero";
        case CrystalBlock::Devil29: return "devil29_bit0";
    }
    return "?";
}
inline std::string CrystalGateText(const CrystalGate& g) {
    auto number = [](const std::optional<int32_t>& v) { return v ? std::to_string(*v) : std::string("-"); };
    auto real = [](const std::optional<double>& v) {
        char text[32] = "-";
        if (v) std::snprintf(text, sizeof(text), "%.3f", *v);
        return std::string(text);
    };
    char devil[16] = "-";
    if (g.devil) std::snprintf(devil, sizeof(devil), "%08X", *g.devil);
    return "modo=" + number(g.mode) + " devil=" + devil + " indice=" + number(g.devil_index) +
           " hector73=" + number(g.hector73) + " devil29=" + number(g.devil29) + " chance=" + real(g.chance) +
           " sorteio=" + real(g.roll) + " soltou=" + (g.spawned ? "1" : "0") + " -> " +
           CrystalBlockName(CrystalBlockOf(g));
}
// Log: os 12 primeiros inimigos, toda troca de motivo e 1 a cada 50; no maximo 200 linhas.
inline constexpr uint32_t kMaxCrystalLogs = 200;
inline bool LogCrystalGate(uint32_t kill_number, bool block_changed, uint32_t logged) {
    return logged < kMaxCrystalLogs && (kill_number <= 12 || block_changed || kill_number % 50 == 0);
}
// Gravacoes que ligam ou desligam o cristal: atributo 73 do Hector e atributo 29 de qualquer Devil.
inline constexpr bool CrystalAttribute(uint32_t id, uint32_t stat) {
    return (id == 0 && stat == kAttrHectorCrystal) || stat == kAttrDevilCrystal;
}
inline std::string CrystalSummaryText(const uint32_t (&kills)[kCrystalBlocks], uint32_t spawned,
                                      const std::string& last, uint32_t writes, const std::string& last_write) {
    uint32_t total = 0;
    for (uint32_t count : kills) total += count;
    std::string text = "Cristais: " + std::to_string(total) + " inimigos |";
    for (int i = 0; i < kCrystalBlocks; ++i)
        text += std::string(i ? ", " : " ") + CrystalBlockName(CrystalBlock(i)) + " " + std::to_string(kills[i]);
    text += " | soltou " + std::to_string(spawned) + "\nCristal (ultimo inimigo): " + last +
            "\nGravacoes atributo 73/29: " + std::to_string(writes) + " | ultima: " + last_write;
    return text;
}
}
