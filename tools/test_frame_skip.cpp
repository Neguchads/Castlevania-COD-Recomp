#include "../src/common/frame_skip_rules.h"
#include <cstdio>

using namespace cod_graphics;

static unsigned Shown(float speed, unsigned frames) {
    unsigned n = 0;
    for (unsigned i = 0; i < frames; i++) n += ShowFrame(i, speed);
    return n;
}

int main() {
    // 1x ou menos: todo quadro aparece
    for (unsigned i = 0; i < 8; i++) if (!ShowFrame(i, 1) || !ShowFrame(i, .5f)) return 1;
    // 2x: 1 de 2; 4x: 1 de 4
    if (!ShowFrame(0, 2) || ShowFrame(1, 2) || !ShowFrame(2, 2)) return 2;
    if (!ShowFrame(0, 4) || ShowFrame(1, 4) || ShowFrame(2, 4) || ShowFrame(3, 4) || !ShowFrame(4, 4)) return 3;
    // a tela recebe quadros / velocidade em qualquer velocidade: 1,5x mostra 2 de 3, 3x 1 de 3, 1,1x 10 de 11
    if (Shown(1.5f, 300) != 200 || Shown(3, 300) != 100 || Shown(2, 300) != 150 || Shown(1.1f, 1100) != 1000) return 4;
    // valores estranhos nao travam a tela
    if (!ShowFrame(5, 0) || !ShowFrame(5, -3)) return 5;
    // quadro pulado: so o que vai para o back buffer sai; texturas renderizadas continuam
    if (!SkipDraw(true, 0x100, 0x100) || SkipDraw(false, 0x100, 0x100) || SkipDraw(true, 0x200, 0x100) ||
        SkipDraw(true, 0x100, 0)) return 6;
    // visibilidade num quadro pulado repete a ultima medida; sem medida anterior fica o que mediu
    if (ReportCount(true, 0, true, 37) != 37 || ReportCount(false, 12, true, 37) != 12 || ReportCount(true, 5, false, 37) != 5)
        return 7;
    std::puts("Frame skip: 1x mostra tudo, acima disso a tela recebe quadros/velocidade, so o back buffer e pulado, "
              "visibilidade repetida");
}
