#include <cmath>
#include <cstdio>
#include <string>
#include "../src/common/drop_rules.h"

using namespace dev;

int main() {
    if (DropRoll(.4, 1) != .4 || DropRoll(.4, 2) != .2) return 1;
    if (DropRoll(.4, 0) < 1e29) return 2;                     // 0x: nunca solta
    if (DropRoll(.5, 50) != .05 || DropRoll(.5, NAN) != .5) return 3;   // limite 10x, NaN neutro
    if (DropCheckAt(0x8F102) != DropCheck::Item || DropCheckAt(0x8F187) != DropCheck::Item) return 4;
    if (DropCheckAt(0x8F263) != DropCheck::Crystal || DropCheckAt(0x8F264) != DropCheck::None) return 5;
    if (!ItemDropReturn(0x8F130) || !ItemDropReturn(0x8F1B5) || ItemDropReturn(0x8F2A3)) return 6;
    if (CrystalAmount(3, 1) != 3 || CrystalAmount(3, 2) != 6 || CrystalAmount(3, 9) != 15) return 7;
    if (CrystalAmount(0, 5) != 0 || CrystalAmount(-1, 5) != -1 || CrystalAmount(4, .2f) != 4) return 8;
    const int16_t table[]{0, -3, 12, 0, 40};
    if (PickDropItem(table, 5, 0, 7) != 12 || PickDropItem(table, 5, .99f, 7) != 40) return 9;
    if (PickDropItem(table, 5, .5f, 7) != 12) return 10;      // .5 * 5 entradas = entrada 2
    const int16_t empty[]{0, -1};
    if (PickDropItem(empty, 2, .3f, 7) != 7 || PickDropItem(nullptr, 0, .3f, 7) != 7) return 11;
    if (ClampItemScale(.1f) != kMinItemScale || ClampItemScale(5) != kMaxItemScale || ClampItemScale(NAN) != 1) return 12;

    // Diagnostico do cristal: endereco de retorno -> condicao, na ordem do jogo.
    if (CrystalProbeAt(0x8EF50) != CrystalProbe::DevilIndex || CrystalProbeAt(0x8EFBD) != CrystalProbe::Mode) return 13;
    if (CrystalProbeAt(0x8EFC7) != CrystalProbe::Devil || CrystalProbeAt(0x8EFD7) != CrystalProbe::Hector73) return 14;
    if (CrystalProbeAt(0x8EFEA) != CrystalProbe::Devil29 || CrystalProbeAt(0x8F255) != CrystalProbe::Chance) return 15;
    if (CrystalProbeAt(0x8EF5A) != CrystalProbe::None || CrystalProbeAt(0x8F263) != CrystalProbe::None) return 16;
    CrystalGate gate;
    if (CrystalBlockOf(gate) != CrystalBlock::NotReached) return 17;
    RecordCrystalProbe(gate, CrystalProbe::DevilIndex, 0);
    RecordCrystalProbe(gate, CrystalProbe::Mode, 2);
    if (CrystalBlockOf(gate) != CrystalBlock::Mode) return 18;                 // Trevor/modo especial
    RecordCrystalProbe(gate, CrystalProbe::Mode, 0);
    RecordCrystalProbe(gate, CrystalProbe::Devil, 0);
    if (CrystalBlockOf(gate) != CrystalBlock::NoDevil) return 19;              // sem Devil invocado
    RecordCrystalProbe(gate, CrystalProbe::Devil, 0x00B30000);
    RecordCrystalProbe(gate, CrystalProbe::Hector73, 0);
    if (CrystalBlockOf(gate) != CrystalBlock::Hector73) return 20;             // relato do testador
    const std::string text = CrystalGateText(gate);
    if (text.find("modo=0 devil=00B30000 indice=0 hector73=0 devil29=-") != 0 ||
        text.find("-> hector73_zero") == std::string::npos) return 21;
    RecordCrystalProbe(gate, CrystalProbe::Hector73, 1);
    if (CrystalBlockOf(gate) != CrystalBlock::NotReached) return 22;           // atributo 29 ainda nao lido
    RecordCrystalProbe(gate, CrystalProbe::Devil29, 3);
    if (CrystalBlockOf(gate) != CrystalBlock::Devil29) return 23;              // bit 0 = nao evoluir
    RecordCrystalProbe(gate, CrystalProbe::Devil29, 2);
    if (CrystalBlockOf(gate) != CrystalBlock::Allowed) return 24;
    RecordCrystalProbe(gate, CrystalProbe::DevilIndex, uint32_t(-1));
    if (CrystalBlockOf(gate) != CrystalBlock::NoDevil) return 25;              // indice negativo
    gate.chance = .25; gate.roll = .5; gate.spawned = true;
    if (CrystalGateText(gate).find("chance=0.250 sorteio=0.500 soltou=1") == std::string::npos) return 26;
    if (!LogCrystalGate(12, false, 11) || LogCrystalGate(13, false, 12) || !LogCrystalGate(13, true, 12)) return 27;
    if (!LogCrystalGate(100, false, 40) || LogCrystalGate(100, true, kMaxCrystalLogs)) return 28;
    if (!CrystalAttribute(0, 0x49) || CrystalAttribute(1, 0x49) || !CrystalAttribute(3, 0x1D) || CrystalAttribute(0, 6)) return 29;
    const uint32_t kills[kCrystalBlocks]{0, 0, 0, 2, 9, 0};
    const std::string summary = CrystalSummaryText(kills, 0, "x", 1, "y");
    if (summary.find("Cristais: 11 inimigos | liberado 0, nao_chegou 0, modo_especial 0, sem_devil 2, hector73_zero 9, devil29_bit0 0 | soltou 0") != 0 ||
        summary.find("Gravacoes atributo 73/29: 1 | ultima: y") == std::string::npos) return 30;
    std::puts("Drop rules: roll factor, check addresses, crystal amount, random table item, item scale and crystal gate passed");
}
