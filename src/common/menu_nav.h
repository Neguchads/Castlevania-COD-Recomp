#pragma once
#include <imgui.h>
#include <imgui_internal.h>

namespace dev {
// Menu dev em niveis: abas (L1/R1) -> titulos -> opcoes -> ajuste (item ativo do ImGui).
struct MenuNav {
    enum class Level { Titles, Options };
    Level level=Level::Titles;
    int tab=0,title=0;
    bool focus=true;  // leva o foco ao nivel atual no proximo quadro
};
// busy: ajustando um item ou com popup aberto; nesse caso B e L1/R1 ficam com o ImGui.
struct MenuNavInput { int tab_step=0; bool back=false; bool busy=false; };

inline void StepMenuNav(MenuNav& nav,const MenuNavInput& input,int tab_count) {
    if(input.busy || tab_count<=0) return;
    if(input.tab_step) {
        nav.tab=((nav.tab+input.tab_step)%tab_count+tab_count)%tab_count;
        nav.title=0;nav.level=MenuNav::Level::Titles;nav.focus=true;
        return;
    }
    // B nos titulos nao fecha: so devolve o foco ao titulo (o ImGui o leva para a janela-mae).
    if(input.back) { nav.level=MenuNav::Level::Titles;nav.focus=true; }
}
inline void EnterMenuOptions(MenuNav& nav,int title) {
    nav.title=title;nav.level=MenuNav::Level::Options;nav.focus=true;
}
// Um titulo da coluna esquerda; A ou clique entra nas opcoes dele.
inline bool MenuTitle(MenuNav& nav,int index,const char* label,ImVec2 size) {
    const bool chosen=ImGui::Selectable(label,nav.title==index,ImGuiSelectableFlags_None,size);
    if(nav.focus && nav.level==MenuNav::Level::Titles && index==nav.title) {
        ImGui::SetFocusID(ImGui::GetItemID(),ImGui::GetCurrentWindow());
        ImGui::GetCurrentContext()->NavDisableHighlight=false;
        nav.focus=false;
    }
    if(chosen) EnterMenuOptions(nav,index);
    return chosen;
}
// Chamar logo apos abrir a coluna de opcoes: foca o primeiro item e aceita clique do mouse.
inline void MenuOptionsFocus(MenuNav& nav) {
    ImGuiWindow* window=ImGui::GetCurrentWindow();
    if(nav.focus && nav.level==MenuNav::Level::Options) {
        ImGui::FocusWindow(window);
        ImGui::NavInitWindow(window,true);
        nav.focus=false;
    }
    if(nav.level==MenuNav::Level::Titles && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
       ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) nav.level=MenuNav::Level::Options;
}
}
