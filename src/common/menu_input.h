#pragma once
#include "core/input.h"
#include "dev_rules.h"
#include <imgui.h>

namespace dev {
inline int MenuSectionStep(bool editing, bool popup) {
    if(editing || popup) return 0;
    const bool left=ImGui::IsKeyPressed(ImGuiKey_GamepadDpadLeft,false);
    const bool right=ImGui::IsKeyPressed(ImGuiKey_GamepadDpadRight,false);
    return int(right)-int(left);
}
inline void MenuGamePad(input::Pad& pad, bool character_selection) {
    // Manter a porta virtual presente tambem para quem joga so com teclado.
    pad.connected=true;
    if (character_selection) {
        pad={};pad.connected=true;
        return;
    }
    // Analógicos seguem para o jogo; botoes ficam reservados para a interface.
    pad.buttons=0;
    for(auto& button:pad.analog) button=0;
}
inline void QueueMenuPad(input::Pad pad, bool active, dev_rules::ReleaseGate& back, bool adjusting=false) {
    if(!active) pad={};
    // Ajustando uma barra, o analogico esquerdo tambem arrasta (meio curso, para ignorar drift).
    if(adjusting) {
        constexpr int kStickStep=16000;
        if(pad.lx>kStickStep) pad.buttons|=8;
        if(pad.lx<-kStickStep) pad.buttons|=4;
        if(pad.ly>kStickStep) pad.buttons|=1;
        if(pad.ly<-kStickStep) pad.buttons|=2;
    }
    auto& io=ImGui::GetIO();
    if(pad.connected) io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
    else io.BackendFlags&=~ImGuiBackendFlags_HasGamepad;
    io.AddKeyEvent(ImGuiKey_GamepadFaceDown,pad.analog[0]!=0);
    io.AddKeyEvent(ImGuiKey_GamepadFaceRight,back.Filter(active,pad.analog[1]!=0));
    io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,pad.analog[2]!=0);
    io.AddKeyEvent(ImGuiKey_GamepadFaceUp,pad.analog[3]!=0);
    io.AddKeyEvent(ImGuiKey_GamepadL1,pad.analog[5]!=0);
    io.AddKeyEvent(ImGuiKey_GamepadR1,pad.analog[4]!=0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadUp,(pad.buttons&1)!=0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadDown,(pad.buttons&2)!=0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadLeft,(pad.buttons&4)!=0);
    io.AddKeyEvent(ImGuiKey_GamepadDpadRight,(pad.buttons&8)!=0);
    // Nunca converter movimento em navegacao, mesmo com drift ou stick segurado.
    for(auto key:{ImGuiKey_GamepadLStickLeft,ImGuiKey_GamepadLStickRight,
                  ImGuiKey_GamepadLStickUp,ImGuiKey_GamepadLStickDown})
        io.AddKeyAnalogEvent(key,false,0);
}
}
