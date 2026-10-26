// Botoes PT-BR / EN do menu dev com ImGui real, sem janela: clique troca na hora, so uma vez,
// cabem na largura anunciada e o texto do menu acompanha o idioma.
#include "../src/common/menu_language_ui.h"
#include "../src/common/menu_language.h"
#include <cstdio>
#include <cstring>

int main() {
    ImGui::CreateContext();
    auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={800,600};io.DeltaTime=1.0f/60;
    unsigned char* pixels=nullptr;int width=0,height=0;
    io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    constexpr float scale=1.5f;
    bool english=false;
    int changes=0;
    ImVec2 pt_min{},pt_max{},en_min{},en_max{};
    const char* label=nullptr;
    auto frame=[&] {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({700,400});
        ImGui::Begin("Menu",nullptr,ImGuiWindowFlags_NoDecoration);
        ImGui::SetCursorPosX(700-dev::LanguageButtonsWidth(scale)-24*scale);
        const ImVec2 start=ImGui::GetCursorScreenPos();
        if(dev::LanguageButtons(english,scale)) ++changes;
        en_min=ImGui::GetItemRectMin();en_max=ImGui::GetItemRectMax();
        pt_min=start;pt_max={start.x+58*scale,start.y+26*scale};
        label=dev::MenuText("Salvar opcoes",english);
        ImGui::End();
        ImGui::Render();
    };
    auto click=[&](ImVec2 a,ImVec2 b) {
        io.AddMousePosEvent((a.x+b.x)*.5f,(a.y+b.y)*.5f);frame();
        io.AddMouseButtonEvent(0,true);frame();
        io.AddMouseButtonEvent(0,false);frame();
        frame();
    };
    using dev::LanguageButtonsWidth;
    frame();
    frame();
    // 1: comeca em portugues
    if(english || changes || std::strcmp(label,"Salvar opcoes")!=0) return 1;
    // 2: os dois botoes cabem exatamente na largura anunciada (alinhamento a direita do cabecalho)
    if(en_max.x-pt_min.x>LanguageButtonsWidth(scale)+.5f || en_max.x>700) return 2;
    // 3: clicar EN troca uma vez e o texto do menu ja sai em ingles
    click(en_min,en_max);
    if(!english || changes!=1 || std::strcmp(label,"Save settings")!=0) return 3;
    // 4: clicar EN de novo nao conta como troca
    click(en_min,en_max);
    if(!english || changes!=1) return 4;
    // 5: PT-BR volta
    click(pt_min,pt_max);
    if(english || changes!=2 || std::strcmp(label,"Salvar opcoes")!=0) return 5;
    ImGui::DestroyContext();
    std::printf("Menu language buttons: click switches once, fit the header and retranslate passed\n");
    return 0;
}
