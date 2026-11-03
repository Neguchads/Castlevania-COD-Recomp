#include "gpu/texture.h"
#include <array>
#include <cstdio>

int ValidateTextureColors() {
    // Formatos reais do SDK: pixels com vermelho e azul distintos detectam inversão.
    struct Case { uint32_t format; std::array<uint8_t, 4> input, bgra; };
    const Case cases[] = {
        {0x12, {0x11, 0x22, 0xCC, 0xFF}, {0x11, 0x22, 0xCC, 0xFF}},
        {0x1E, {0x11, 0x22, 0xCC, 0x00}, {0x11, 0x22, 0xCC, 0xFF}},
        {0x3F, {0xCC, 0x22, 0x11, 0xFF}, {0x11, 0x22, 0xCC, 0xFF}},
        {0x40, {0xFF, 0xCC, 0x22, 0x11}, {0x11, 0x22, 0xCC, 0xFF}},
        {0x41, {0xFF, 0x11, 0x22, 0xCC}, {0x11, 0x22, 0xCC, 0xFF}},
    };
    for (const Case& test : cases) {
        gpu::TextureDesc desc;
        desc.format = test.format;
        desc.width = desc.height = 1;
        desc.pitch = 4;
        desc.linear = true;
        std::vector<uint8_t> decoded;
        uint32_t pitch = 0;
        gpu::DecodeLevel(desc, 0, 0, test.input.data(), nullptr, decoded, pitch);
        if (pitch != 4 || decoded.size() != 4) return 1;
        for (unsigned channel = 0; channel < 4; channel++) {
            if (decoded[channel] != test.bgra[channel]) {
                std::printf("Falha formato %X canal %u\n", test.format, channel);
                return 2;
            }
        }
    }
    if (gpu::PackColor({1, 0, 0, 1}) != 0xFFFF0000) return 3;
    if (gpu::PackColor({0, 0, 1, 1}) != 0xFF0000FF) return 4;
    gpu::TextureDesc indexed;
    indexed.format = 0x0B;
    indexed.width = indexed.height = 1;
    std::array<uint32_t, 256> palette{};
    palette[7] = 0x80CC2211;
    const std::array<uint8_t, 2> index{7, 0};
    std::vector<uint8_t> decoded;
    uint32_t pitch = 0;
    gpu::DecodeLevel(indexed, 0, 0, index.data(), palette.data(), decoded, pitch);
    if (decoded != std::vector<uint8_t>({0x11, 0x22, 0xCC, 0x80})) return 5;
    std::puts("Cores: conversoes de textura BGRA/ARGB/RGBA e constantes passaram");
    return 0;
}
