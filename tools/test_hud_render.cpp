#include "../src/common/hud_render_rules.h"
#include <cmath>
#include <initializer_list>
#include <cstdio>
int main() {
    if(!cod_hud::PetTextureSet(64,64,256,256,0x1000,256,256,0x1000))return 9;
    // Tres desenhos do logo na captura221903: mesmos shaders, outras texturas.
    if(cod_hud::PetTextureSet(256,256,256,256,0x1000,512,512,0x2000) ||
       cod_hud::PetTextureSet(64,64,256,256,0x1000,256,256,0x2000) ||
       cod_hud::PetTextureSet(64,64,256,256,0,256,256,0))return 10;
    std::puts("Pet highlight: title-logo shared shaders excluded by captured texture set");
    if(!cod_hud::KeepVisible(true,true,false) || cod_hud::KeepVisible(false,true,false) ||
       cod_hud::KeepVisible(true,false,false) || cod_hud::KeepVisible(true,true,true))return 7;
    float timer=0;uint8_t alpha=0;uint32_t state=0xA5A5FFFFu;
    cod_hud::RefreshVisibility(timer,alpha,state);
    if(timer!=5 || alpha!=0x80 || (state&0x1E)!=8 || (state&~0x1Eu)!=(0xA5A5FFFFu&~0x1Eu))return 8;
    std::puts("HUD always visible: enable/menu guards, native timeout/alpha and unrelated flags passed");
    if(!cod_hud::PetHighlight(0x6B7F98AECCA424BDull,0xB88574E78D5DD008ull,[]{return false;}))return 1;
    if(cod_hud::PetHighlight(0,0xB88574E78D5DD008ull,[]{return false;}) ||
       cod_hud::PetHighlight(0x6B7F98AECCA424BDull,0,[]{return false;}) ||
       cod_hud::PetHighlight(0x6B7F98AECCA424BDull,0xB88574E78D5DD008ull,[]{return true;}))return 2;
    int queries=0;
    auto query=[&]{++queries;return false;};
    for(int i=0;i<100000;++i)cod_hud::PetHighlight(0,0,query);
    if(queries!=0)return 5;
    if(!cod_hud::PetHighlight(0x6B7F98AECCA424BDull,0xB88574E78D5DD008ull,query) || queries!=1)return 6;
    std::puts("Pet highlight: 100000 unrelated draws = 0 memory queries; matching draw = 1");
    for(float scale:{1.f,.5f,.35f}) {
        float m[]{1.25f,1.07143f,1,1},o[]{.53125f,.53125f,0,0};
        const float before_x=16*m[0]+o[0],before_y=288*m[1]+o[1];
        cod_hud::ScaleHighlight(m,o,scale,480);
        if(std::abs((16*m[0]+o[0])-before_x*scale)>.001f ||
           std::abs((288*m[1]+o[1])-(480+(before_y-480)*scale))>.001f)return 3;
        if(m[2]!=1 || m[3]!=1 || o[2]!=0 || o[3]!=0)return 4;
    }
    std::puts("Pet highlight: shader guards, bottom-left anchor and depth preserved");
}
