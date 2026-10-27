#include "../src/common/menu_nav.h"
#include "../src/common/menu_input.h"
#include <cstdio>

// Navegacao do menu dev com ImGui real, sem janela: abas -> titulos -> opcoes -> ajuste.
int main() {
    using dev::MenuNav;
    // Logica pura: L1/R1 trocam de aba e voltam aos titulos; B sobe um nivel; ajuste/popup bloqueiam.
    MenuNav pure;
    dev::EnterMenuOptions(pure,1);
    if(pure.level!=MenuNav::Level::Options || pure.title!=1 || !pure.focus) return 1;
    pure.focus=false;
    dev::StepMenuNav(pure,{.back=true,.busy=true},3);
    if(pure.level!=MenuNav::Level::Options) return 2;
    dev::StepMenuNav(pure,{.back=true},3);
    if(pure.level!=MenuNav::Level::Titles || pure.title!=1 || !pure.focus) return 3;
    dev::StepMenuNav(pure,{.tab_step=-1},3);
    if(pure.tab!=2 || pure.title!=0 || pure.level!=MenuNav::Level::Titles) return 4;
    dev::StepMenuNav(pure,{.tab_step=1,.busy=true},3);
    if(pure.tab!=2) return 5;

    ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={800,600};io.DeltaTime=1.0f/60;
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_NavEnableGamepad;
    unsigned char* pixels=nullptr;int width=0,height=0;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    dev_rules::ReleaseGate gate;
    input::Pad pad{};pad.connected=true;
    MenuNav nav;
    bool check=false,was_busy=false;
    float value=.5f;
    ImGuiID title_ids[2]{},check_id=0,slider_id=0;
    const char* titles[2][2]{{"Camera","HUD"},{"Volume",nullptr}};
    auto frame=[&] {
        dev::QueueMenuPad(pad,true,gate,was_busy);
        ImGui::NewFrame();
        const bool back=ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight,false);
        const int step=int(ImGui::IsKeyPressed(ImGuiKey_GamepadR1,false))-int(ImGui::IsKeyPressed(ImGuiKey_GamepadL1,false));
        dev::StepMenuNav(nav,{.tab_step=step,.back=back,.busy=was_busy},2);
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({700,500});
        ImGui::Begin("Menu",nullptr,ImGuiWindowFlags_NoDecoration);
        ImGui::BeginChild("titles",{200,0},ImGuiChildFlags_Border);
        for(int i=0;i<2 && titles[nav.tab][i];++i) {
            dev::MenuTitle(nav,i,titles[nav.tab][i],{0,30});
            title_ids[i]=ImGui::GetItemID();
        }
        ImGui::EndChild();ImGui::SameLine();
        ImGui::BeginChild("content",{0,0},ImGuiChildFlags_Border);
        dev::MenuOptionsFocus(nav);
        ImGui::Checkbox("Opcao",&check);check_id=ImGui::GetItemID();
        ImGui::SliderFloat("Barra",&value,0,1);slider_id=ImGui::GetItemID();
        ImGui::EndChild();
        ImGui::End();
        was_busy=ImGui::IsAnyItemActive();
        ImGui::Render();
    };
    auto press=[&](uint16_t buttons,int analog=-1) {
        pad.buttons=buttons;if(analog>=0) pad.analog[analog]=255;
        for(int i=0;i<2;++i) frame();
        pad.buttons=0;for(auto& button:pad.analog) button=0;
        for(int i=0;i<2;++i) frame();
    };
    auto focus=[] { return ImGui::GetCurrentContext()->NavId; };
    for(int i=0;i<3;++i) frame();
    if(focus()!=title_ids[0]) return 10;
    press(2);                       // direcional baixo: segundo titulo, sem entrar nas opcoes
    if(focus()!=title_ids[1] || nav.level!=MenuNav::Level::Titles) return 11;
    press(0,0);                     // A entra nas opcoes do titulo, sem acionar a primeira opcao
    if(nav.level!=MenuNav::Level::Options || nav.title!=1 || focus()!=check_id || check) return 12;
    press(2);
    if(focus()!=slider_id) return 13;
    press(0,0);                     // A na barra entra no ajuste
    if(ImGui::GetCurrentContext()->ActiveId!=slider_id) return 14;
    press(8);                       // direcional direita ajusta
    if(value<=.5f) return 15;
    const float after_dpad=value;
    pad.lx=30000;for(int i=0;i<20;++i) frame();pad.lx=0;frame();   // analogico tambem arrasta
    if(value<=after_dpad) return 16;
    press(0,1);                     // B sai do ajuste e fica nas opcoes
    if(ImGui::GetCurrentContext()->ActiveId || nav.level!=MenuNav::Level::Options) return 17;
    press(0,1);                     // B volta aos titulos, no titulo que estava
    if(nav.level!=MenuNav::Level::Titles || focus()!=title_ids[1]) return 18;
    press(0,1);                     // B nos titulos nao sai do menu nem perde o foco
    if(nav.level!=MenuNav::Level::Titles || focus()!=title_ids[1]) return 19;
    press(0,4);                     // R1 (indice 4) troca de aba
    if(nav.tab!=1 || focus()!=title_ids[0]) return 20;
    pad.lx=30000;for(int i=0;i<5;++i) frame();pad.lx=0;frame();      // fora do ajuste, analogico nao navega
    if(focus()!=title_ids[0]) return 21;
    dev::QueueMenuPad(pad,false,gate);
    ImGui::DestroyContext();
    std::puts("Menu nav: titles, options, slider adjust by dpad/stick, back levels and tab switch passed");
}
