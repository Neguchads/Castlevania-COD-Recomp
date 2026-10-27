#pragma once
#include <cmath>
#include <cstdint>

namespace cod_graphics {
// Speedhack: o COD avanca um passo de logica por quadro, entao 2x pede o dobro de quadros por segundo.
// Com a imagem cara (resolucao interna alta) o PC nao chega la e a velocidade trava perto de 1x
// (F3 24/10: 2x pedido, 63 FPS). Acima de 1x o jogo roda todos os quadros e a tela recebe so o limite de FPS:
// o quadro aparece quando floor(quadro / velocidade) avanca (2x: 1 de 2; 1,5x: 2 de 3; 4x: 1 de 4).
inline bool ShowFrame(uint64_t frame, float speed) {
    if (!(speed > 1) || frame == 0) return true;
    const double s = speed;
    return std::floor(double(frame) / s) != std::floor(double(frame - 1) / s);
}
// Num quadro pulado sai so o que vai para o back buffer ou sai dele (cena, HUD, copia final);
// texturas renderizadas (que o jogo pode desenhar uma vez e reaproveitar) continuam sendo desenhadas.
inline bool SkipDraw(bool skipping, uint32_t target_header, uint32_t back_buffer) {
    return skipping && back_buffer != 0 && target_header == back_buffer;
}
// Teste de visibilidade (brilho, lens flare) num quadro pulado: o desenho nao rodou, entao repete o ultimo
// resultado medido naquele endereco em vez de dizer "0 pixels" e piscar.
inline uint32_t ReportCount(bool skipped, uint32_t measured, bool has_last, uint32_t last) {
    return skipped && has_last ? last : measured;
}
}
