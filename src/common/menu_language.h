#pragma once
#include <string_view>
#include <unordered_map>
#include "menu_strings_en.h"

namespace dev {
// Idioma do menu dev (so as ferramentas; o idioma do jogo e outra opcao). Chave menu_language: "pt" ou "en".
inline bool MenuLanguageIsEnglish(std::string_view key) { return key=="en"; }
inline const char* MenuLanguageKey(bool english) { return english ? "en" : "pt"; }

// Ingles quando pedido e conhecido; senao o proprio texto, para uma traducao faltando nunca apagar o rotulo.
inline const char* MenuText(const char* pt, bool english) {
    if(!english || !pt) return pt;
    static const auto table=[] {
        std::unordered_map<std::string_view,const char*> map;
        for(const auto& [key,value]:kMenuStringsEn) map.emplace(key,value);
        return map;
    }();
    const auto found=table.find(pt);
    return found==table.end() ? pt : found->second;
}
}
