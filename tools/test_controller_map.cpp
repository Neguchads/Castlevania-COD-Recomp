#include "../src/common/controller_map.h"
#include <cstdio>

int main() {
    using namespace dev;
    // Padrao: nada muda.
    input::Pad pad;
    pad.analog[0] = 255; pad.analog[6] = 90; pad.buttons = 1u << 4 | 1u << 9; pad.lx = 1234;
    const auto same = RemapPad(pad, IdentityControllerMap());
    if (same.analog[0] != 255 || same.analog[6] != 90 || same.buttons != pad.buttons || same.lx != 1234) return 1;
    // A e B trocados: o fisico A vira B no jogo; bits extras e analogicos seguem.
    auto map = IdentityControllerMap();
    AssignButton(map, 0, 1);
    if (map[0] != 1 || map[1] != 0) return 2;
    const auto swapped = RemapPad(pad, map);
    if (swapped.analog[0] != 0 || swapped.analog[1] != 255 || (swapped.buttons & (1u << 9)) == 0 || swapped.lx != 1234) return 3;
    // Gatilho como Start: o jogo recebe Start so com meio curso; botao digital como gatilho vira 255.
    map = IdentityControllerMap();
    AssignButton(map, 12, 7);  // Start <- RT
    input::Pad trigger;
    trigger.analog[7] = 100;
    if (RemapPad(trigger, map).buttons & (1u << 4)) return 4;
    trigger.analog[7] = 200;
    const auto start = RemapPad(trigger, map);
    if (!(start.buttons & (1u << 4)) || start.analog[7] != 0) return 5;
    trigger = {}; trigger.buttons = 1u << 4;  // Start fisico agora aciona RT
    if (RemapPad(trigger, map).analog[7] != 255) return 6;
    // Atribuicao continua sendo permutacao (ninguem perde botao).
    uint32_t seen = 0;
    for (auto source : map) seen |= 1u << source;
    if (seen != 0xFFFF) return 7;
    AssignButton(map, 3, 99);
    AssignButton(map, -1, 2);
    if (map[3] != 3) return 8;
    // Texto salvo: ida e volta; invalido volta ao padrao.
    if (ParseControllerMap(FormatControllerMap(map)) != map) return 9;
    for (const char* bad : {"", "0,1", "0,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15", "a,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15",
                            "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,16", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,0"})
        if (ParseControllerMap(bad) != IdentityControllerMap()) return 10;
    // Captura: ignora o botao que abriu a captura ate soltar, pega o proximo, desiste no prazo.
    ButtonCapture capture;
    capture.Begin(5, 0);
    if (capture.Update(1u << 0, .1).source != -1) return 11;  // A ainda apertado
    if (capture.Update(0, .2).source != -1) return 12;
    const auto picked = capture.Update(1u << 14 | 1u << 15, .3);
    if (picked.target != 5 || picked.source != 14 || capture.target != -1) return 13;
    capture.Begin(2, 1);
    capture.Update(0, 2);
    if (capture.Update(1u << 3, 8).source != -1 || capture.target != -1) return 14;
    input::Pad held; held.analog[6] = 127; held.analog[2] = 255; held.buttons = 1u << 7;
    if (PadHeldMask(held) != (1u << 2 | 1u << 15)) return 15;
    // Layout PS2: L1 fisico (LB) aciona o gatilho L do jogo, L2 fisico (LT) aciona o preto; segue permutacao valida.
    const auto ps2 = Ps2ControllerMap();
    if (ParseControllerMap(FormatControllerMap(ps2)) != ps2 || ps2 == IdentityControllerMap()) return 16;
    input::Pad shoulders; shoulders.analog[5] = 255; shoulders.analog[6] = 200;  // LB inteiro, LT a 200
    const auto ps2_pad = RemapPad(shoulders, ps2);
    if (ps2_pad.analog[6] != 255 || ps2_pad.analog[4] != 200 || ps2_pad.analog[5] != 0 || ps2_pad.analog[7] != 0 || ps2_pad.analog[0] != 0) return 17;
    input::Pad right; right.analog[4] = 255; right.analog[7] = 255;  // RB e RT
    const auto ps2_right = RemapPad(right, ps2);
    if (ps2_right.analog[7] != 255 || ps2_right.analog[5] != 255 || ps2_right.analog[4] != 0 || ps2_right.analog[6] != 0) return 18;
    // Layout atual: Xbox, PS2 ou personalizado (qualquer troca a mais).
    if (DetectControllerLayout(IdentityControllerMap()) != ControllerLayout::Xbox || DetectControllerLayout(ps2) != ControllerLayout::Ps2) return 19;
    auto custom = ps2; AssignButton(custom, 0, 1);
    if (DetectControllerLayout(custom) != ControllerLayout::Custom || DetectControllerLayout(map) != ControllerLayout::Custom) return 20;
    std::puts("Controller map: identity, swap, trigger as button, permutation, save text, capture, PS2 layout and layout detection passed");
}
