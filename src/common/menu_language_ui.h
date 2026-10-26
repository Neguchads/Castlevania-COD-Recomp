#pragma once
#include <initializer_list>
#include <imgui.h>

namespace dev {
// Botoes PT-BR / EN do canto superior direito do menu dev. Largura total, para alinhar a direita.
inline float LanguageButtonsWidth(float scale) { return (58+44+4)*scale; }
// Desenha os dois botoes na linha atual; o idioma ativo fica destacado. Devolve true so no clique que troca.
inline bool LanguageButtons(bool& english,float scale) {
    bool changed=false;
    for(const bool option:{false,true}) {
        if(option) ImGui::SameLine(0,4*scale);
        const bool current=english==option;
        if(current) ImGui::PushStyleColor(ImGuiCol_Button,ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
        if(ImGui::Button(option ? "EN" : "PT-BR",{(option ? 44.0f : 58.0f)*scale,26*scale}) && !current) {
            english=option;
            changed=true;
        }
        if(current) ImGui::PopStyleColor();
        if(ImGui::IsItemHovered()) ImGui::SetTooltip(option ? "Menu in English" : "Menu em português");
    }
    return changed;
}
}
