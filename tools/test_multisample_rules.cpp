#include "../src/common/multisample_rules.h"
#include <cstdio>

int main() {
    using namespace multisample_rules;
    // 1X ou superficie swizzled: grade igual a memoria.
    auto f = ModeFactor(0, false);
    if (f.x != 1 || f.y != 1) return 1;
    f = ModeFactor(2, true);
    if (f.x != 1 || f.y != 1) return 2;
    // 4X (SQUARE_OFFSET_4) e 2X (CENTER_CORNER_2), como sub_003B9EC0.
    f = ModeFactor(2, false);
    if (f.x != 2 || f.y != 2) return 3;
    f = ModeFactor(1, false);
    if (f.x != 2 || f.y != 1) return 4;
    // Tamanho logico arredonda para cima, como (w + 1) >> 1.
    if (Logical(640, 2) != 320 || Logical(480, 2) != 240 || Logical(641, 2) != 321 || Logical(480, 1) != 480) return 5;
    // Modo do back buffer vem de MULTISAMPLEMODE; de outro alvo, de MULTISAMPLERENDERTARGETMODE.
    if (EffectiveMode(true, 2, 0) != 2 || EffectiveMode(false, 2, 0) != 0 || EffectiveMode(false, 0, 1) != 1) return 6;
    // Volume de sombra do COD: transformado para 320x240 com o modo 4X, cobre o alvo 640x480 inteiro.
    const auto shadow = ModeFactor(EffectiveMode(true, 2, 0), false);
    if (Logical(640, shadow.x) != 320 || Logical(480, shadow.y) != 240) return 7;
    std::puts("Multisample rules: 1X, swizzled, 2X/4X factors, logical size rounding, back buffer vs render target mode passed");
}
