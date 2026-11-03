#pragma once
#include <cstdint>
#include <string_view>

namespace dev {
// Imagens, vozes e cenas PT-BR importadas da ISO PS2 (tools/ps2_*.py). O pacote
// segue o idioma escolhido em Opcoes > Idioma, nao a lista de mods.
inline constexpr std::string_view kTranslationTextureMod="PTBR-PS2";
inline constexpr bool ModApplies(std::string_view name,bool enabled,bool portuguese) {
    return name==kTranslationTextureMod ? portuguese : enabled;
}
// Seletor nativo: 0 japones, 1 ingles, 2 PT-BR. O jogo so conhece 0/1; o PT-BR usa
// a faixa de voz inglesa, que o pacote substitui pela dublagem.
inline constexpr int NextLanguage(int language,bool right) {return (language+(right ? 1 : 2))%3;}
inline constexpr uint32_t NativeVoice(int language) {return language==0 ? 0 : 1;}
inline constexpr int AppliedLanguage(bool portuguese,uint32_t voice) {return portuguese ? 2 : voice==1 ? 1 : 0;}
// Ingles e PT-BR tem a mesma voz nativa; sem esta marca o jogo sairia da tela sem
// perguntar "Change language settings. OK?".
inline constexpr bool PortugueseChangePending(int candidate,bool portuguese) {return (candidate==2)!=portuguese;}
// Texturas HD (mods/*/textures): procurar com algum mod marcado, ou com PT-BR ativo
// e PTBR-PS2/textures presente (o pacote segue o idioma, nao a marcacao).
inline constexpr bool HdLookup(bool any_enabled_mod,bool portuguese,bool translation_textures) {
    return any_enabled_mod || (portuguese && translation_textures);
}
}
