#include "../src/common/translation_pack_rules.h"
#include <cstdio>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "../src/common/texture_pack.h"

int main(int argc,char** argv) {
    // PTBR-PS2 segue apenas o idioma; os demais mods seguem a propria marcacao.
    if(dev::ModApplies("PTBR-PS2",true,false)||!dev::ModApplies("PTBR-PS2",false,true))return 1;
    if(!dev::ModApplies("HD",true,false)||dev::ModApplies("HD",false,true))return 2;
    // O pacote antigo virou um mod comum, sem ligacao com o idioma.
    if(dev::ModApplies("PTBR-Interface",false,true)||!dev::ModApplies("PTBR-Interface",true,false))return 3;
    // Ciclo do seletor nativo: japones, ingles, PT-BR, nos dois sentidos.
    if(dev::NextLanguage(1,true)!=2||dev::NextLanguage(2,true)!=0||dev::NextLanguage(0,false)!=2||
       dev::NextLanguage(1,false)!=0)return 4;
    // PT-BR usa a voz inglesa nativa (dublagem no lugar dela).
    if(dev::NativeVoice(0)!=0||dev::NativeVoice(1)!=1||dev::NativeVoice(2)!=1)return 5;
    if(dev::AppliedLanguage(true,0)!=2||dev::AppliedLanguage(false,1)!=1||dev::AppliedLanguage(false,0)!=0)return 6;
    // Ingles <-> PT-BR nao muda a voz nativa: precisa forcar a confirmacao do jogo.
    if(!dev::PortugueseChangePending(2,false)||!dev::PortugueseChangePending(1,true)||
       dev::PortugueseChangePending(2,true)||dev::PortugueseChangePending(0,false))return 7;
    // HD: PTBR-PS2/textures conta so com PT-BR ativo; mod marcado conta sempre.
    if(!dev::HdLookup(false,true,true)||dev::HdLookup(false,false,true)||dev::HdLookup(false,true,false)||
       !dev::HdLookup(true,false,false))return 10;
    if(argc>1) {
        unsigned count=0;
        for(const auto& entry:std::filesystem::directory_iterator(argv[1])) {
            if(entry.path().extension()!=".bmp")continue;
            std::ifstream stream(entry.path(),std::ios::binary);
            std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(stream)),{});
            const auto decoded=dev::DecodePackBmp(bytes);
            if(!decoded||!decoded->width||!decoded->height)return 8;
            ++count;
        }
        if(!count)return 9;
        std::printf("Translation pack: %u real textures decoded by production loader\n",count);
    }
    std::puts("Translation pack: language-driven PTBR-PS2, native selector cycle and confirmation passed");
}
