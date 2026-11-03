#include "../src/common/indexed_up.h"
#include <array>
#include <cstdio>

int main() {
    const std::array<uint8_t, 6> indices{2, 0, 0, 0, 7, 0};
    if (cod_graphics::IndexedVertexBytes(indices, 16, 1024) != 128) return 1;
    if (cod_graphics::IndexedVertexBytes(indices, 16, 127)) return 2;
    if (cod_graphics::IndexedVertexBytes(indices, 0, 1024)) return 3;
    if (cod_graphics::IndexedVertexBytes({}, 16, 1024)) return 4;
    if (cod_graphics::IndexedVertexBytes(std::span(indices).first(3), 16, 1024)) return 5;
    const std::array<uint8_t, 2> high{255,255};
    if (cod_graphics::IndexedVertexBytes(high, UINT32_MAX, UINT32_MAX)) return 6;
    std::puts("Indexed UP: tamanho, indices e limites passaram");
}
