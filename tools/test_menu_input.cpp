#include "../src/common/menu_input.h"
#include <cstdio>

int main() {
    input::Pad raw{};raw.connected=true;raw.buttons=255;
    raw.lx=32767;raw.ly=-16000;raw.rx=12000;raw.ry=-20000;
    for(auto& button:raw.analog) button=255;
    auto game=raw;dev::MenuGamePad(game,false);
    if(game.buttons || !game.connected || game.lx!=raw.lx || game.ly!=raw.ly ||
       game.rx!=raw.rx || game.ry!=raw.ry) return 1;
    for(auto button:game.analog) if(button) return 2;
    input::Pad keyboard_only{};dev::MenuGamePad(keyboard_only,false);
    if(!keyboard_only.connected) return 9;
    dev::MenuGamePad(game,true);
    if(game.lx || game.ly || game.rx || game.ry || !game.connected) return 3;

    ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={800,600};io.DeltaTime=1.0f/60;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;
    unsigned char* pixels=nullptr;int width=0,height=0;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    dev_rules::ReleaseGate back;
    raw.buttons=0;for(auto& button:raw.analog) button=0;
    bool clicked=false,hovered=false;
    int section_step=0;
    bool editing=false,popup=false;
    auto frame=[&] {
        dev::QueueMenuPad(raw,true,back);
        ImGui::NewFrame();
        if(const int step=dev::MenuSectionStep(editing,popup)) section_step=step;
        ImGui::SetNextWindowPos({20,20});ImGui::SetNextWindowSize({300,200});
        ImGui::Begin("Menu input",nullptr,ImGuiWindowFlags_NoDecoration);
        clicked=ImGui::Button("Confirm",{120,40}) || clicked;
        hovered=ImGui::IsItemHovered();
        ImGui::End();ImGui::Render();
    };
    for(int i=0;i<3;++i) frame();
    if(ImGui::IsKeyDown(ImGuiKey_GamepadLStickRight) || ImGui::IsKeyDown(ImGuiKey_GamepadLStickDown)) return 4;
    io.AddMousePosEvent(60,45);io.AddMouseButtonEvent(0,true);
    for(int i=0;i<3;++i) frame();
    if(!hovered) return 5;
    io.AddMouseButtonEvent(0,false);
    for(int i=0;i<3;++i) frame();
    if(!clicked) return 6;
    raw.buttons=8;
    for(int i=0;i<3;++i) frame();
    if(!ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight) || section_step!=1) return 7;
    section_step=0;
    for(int i=0;i<3;++i) frame();
    if(section_step) return 10;
    raw.buttons=0;io.AddKeyEvent(ImGuiKey_DownArrow,true);
    for(int i=0;i<3;++i) frame();
    if(!ImGui::IsKeyDown(ImGuiKey_DownArrow) || ImGui::IsKeyDown(ImGuiKey_GamepadDpadRight)) return 8;
    editing=true;raw.buttons=4;
    for(int i=0;i<3;++i) frame();
    if(section_step) return 11;
    raw.buttons=0;for(int i=0;i<3;++i) frame();
    editing=false;popup=true;raw.buttons=8;
    for(int i=0;i<3;++i) frame();
    if(section_step) return 12;
    raw.buttons=0;for(int i=0;i<3;++i) frame();
    popup=false;raw.buttons=4;
    for(int i=0;i<3;++i) frame();
    if(section_step!=-1) return 13;
    dev::QueueMenuPad(raw,false,back);
    ImGui::DestroyContext();
    std::puts("Menu input: sticks, mouse, keyboard, left/right sections, editing and popup guards passed");
}
