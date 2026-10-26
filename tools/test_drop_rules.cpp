#include <cmath>
#include <cstdio>
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
    std::puts("Drop rules: roll factor, check addresses, crystal amount, random table item and item scale passed");
}
