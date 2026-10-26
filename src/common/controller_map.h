#pragma once
#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <sstream>
#include <string>
#include "core/input.h"

namespace dev {
// Botoes do controle do Xbox na ordem do mapeamento: os 8 analogicos do Pad (A B X Y preto branco LT RT),
// depois os 8 bits digitais (direcional cima/baixo/esquerda/direita, Start, Back, L3, R3).
inline constexpr int kPadButtonCount = 16;
// destino (botao que o jogo recebe) -> origem (botao fisico apertado)
using ControllerMap = std::array<uint8_t, kPadButtonCount>;
inline constexpr ControllerMap IdentityControllerMap() {
    ControllerMap map{};
    for (int i = 0; i < kPadButtonCount; ++i) map[size_t(i)] = uint8_t(i);
    return map;
}
// Layout do PS2: L1/R1 (fisicos LB/RB) viram os gatilhos L/R do jogo e L2/R2 (fisicos LT/RT) viram preto/branco,
// a mesma ordem dos icones L1 L2 R1 R2 que o pack PS2-Botoes poe no lugar de L preto R branco.
inline constexpr ControllerMap Ps2ControllerMap() {
    ControllerMap map = IdentityControllerMap();
    map[4] = 6;  // preto  <- LT / L2
    map[5] = 7;  // branco <- RT / R2
    map[6] = 5;  // LT     <- LB / L1
    map[7] = 4;  // RT     <- RB / R1
    return map;
}
enum class ControllerLayout { Xbox, Ps2, Custom };
inline constexpr ControllerLayout DetectControllerLayout(const ControllerMap& map) {
    if (map == IdentityControllerMap()) return ControllerLayout::Xbox;
    if (map == Ps2ControllerMap()) return ControllerLayout::Ps2;
    return ControllerLayout::Custom;
}
inline constexpr uint8_t PadButtonValue(const input::Pad& pad, int index) {
    if (index < 8) return pad.analog[index];
    return (pad.buttons >> (index - 8)) & 1 ? 255 : 0;
}
// Botoes apertados (gatilho conta a partir da metade do curso).
inline constexpr uint16_t PadHeldMask(const input::Pad& pad) {
    uint16_t held = 0;
    for (int i = 0; i < kPadButtonCount; ++i) if (PadButtonValue(pad, i) >= 128) held |= uint16_t(1u << i);
    return held;
}
// Analogicos e bits extras passam intactos; cada botao do jogo le o botao fisico escolhido.
inline constexpr input::Pad RemapPad(const input::Pad& pad, const ControllerMap& map) {
    input::Pad out = pad;
    out.buttons &= uint16_t(~0xFFu);
    for (int target = 0; target < kPadButtonCount; ++target) {
        const uint8_t value = PadButtonValue(pad, map[size_t(target)] % kPadButtonCount);
        if (target < 8) out.analog[target] = value;
        else if (value >= 128) out.buttons |= uint16_t(1u << (target - 8));
    }
    return out;
}
// Atribui a origem ao destino e troca com o destino que a usava: todo botao do jogo continua alcancavel.
inline constexpr void AssignButton(ControllerMap& map, int target, int source) {
    if (target < 0 || target >= kPadButtonCount || source < 0 || source >= kPadButtonCount) return;
    for (auto& entry : map) if (entry == source) entry = map[size_t(target)];
    map[size_t(target)] = uint8_t(source);
}
inline std::string FormatControllerMap(const ControllerMap& map) {
    std::string text;
    for (size_t i = 0; i < map.size(); ++i) text += (i ? "," : "") + std::to_string(map[i]);
    return text;
}
// Aceita so uma permutacao completa; qualquer outra coisa volta ao padrao.
inline ControllerMap ParseControllerMap(const std::string& text) {
    ControllerMap map{};
    std::istringstream input(text);
    std::string item;
    uint32_t seen = 0;
    size_t count = 0;
    while (std::getline(input, item, ',')) {
        if (count >= map.size() || item.empty() || item.size() > 2 || item.find_first_not_of("0123456789") != std::string::npos)
            return IdentityControllerMap();
        const int value = std::stoi(item);
        if (value >= kPadButtonCount || (seen >> value) & 1) return IdentityControllerMap();
        seen |= 1u << value;
        map[count++] = uint8_t(value);
    }
    return count == map.size() ? map : IdentityControllerMap();
}
// Espera soltar tudo, depois pega o primeiro botao apertado; desiste no prazo.
struct ButtonCapture {
    struct Result { int target = -1, source = -1; };
    int target = -1;
    bool armed = false;
    double deadline = 0;
    void Begin(int button, double now, double seconds = 6) { target = button; armed = false; deadline = now + seconds; }
    void Cancel() { target = -1; }
    Result Update(uint16_t held, double now) {
        if (target < 0) return {};
        if (now > deadline) { target = -1; return {}; }
        if (!held) { armed = true; return {}; }
        if (!armed) return {};
        const Result result{target, std::countr_zero(held)};
        target = -1;
        return result;
    }
};
}
