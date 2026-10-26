#include "../src/common/presentation_math.h"
#include <cstdio>
#include <limits>
int main() {
    // Painel do inimigo: x=216 no centro; barra (192) a esquerda e "Lv" (240) a direita.
    if(cod_view::EnemyPanelCoordinate(216,.5f)!=0 || cod_view::EnemyPanelCoordinate(216,1)!=0 ||
       cod_view::EnemyPanelCoordinate(192,.5f)!=-12 || cod_view::EnemyPanelCoordinate(240,.5f)!=12 ||
       cod_view::EnemyPanelCoordinate(240,1)!=24)return 11;
    // Nada do painel sai da tela (-256..256) em escalas 0.35..1, mesmo com nome longo.
    for(const float scale:{.35f,.5f,1.0f})
        if(cod_view::EnemyPanelCoordinate(216-200,scale)<-256 || cod_view::EnemyPanelCoordinate(240+120,scale)>256)return 12;
    using namespace cod_view;
    if(HudCoordinate(-200,.5f,true)!=-228 || HudCoordinate(-100,.5f,false)!=-162) return 1;
    if(HudCoordinate(200,.5f,false)!=212 || HudCoordinate(-256,.5f,true)!=-256) return 2;
    if(HudCoordinate(140,1,false)!=140 || HudCoordinate(100,1,true)!=100) return 3;
    if(Focal(1024,2)!=512 || Focal(1024,1)!=1024 || Focal(1024,.5f)!=2048) return 4;
    if(Focal(1024,0)!=1024 || Focal(1024,std::numeric_limits<float>::quiet_NaN())!=1024) return 5;
    const auto forward=Direction(0,0),side=Direction(1.5707963268f,0),up=Direction(0,1.0f);
    if(std::abs(forward.x)>.001f || forward.z!=1 || std::abs(side.x-1)>.001f || up.y<=0) return 6;
    const auto target=OrbitTarget({100,500,200},30);
    if(target.y!=350)return 7;
    for(int i=0;i<4;++i) {
        const float yaw=i*1.5707963268f;
        const auto camera=OrbitPosition(target,yaw,.25f,400);
        if(camera.y>=target.y || std::abs(std::hypot(camera.x-target.x,camera.z-target.z)-400*std::cos(.25f))>.01f)return 8;
        if(std::abs(std::remainder(std::atan2(target.x-camera.x,target.z-camera.z)-yaw,6.28318530718f))>.001f)return 9;
    }
    if(std::abs(FollowYaw(3.13f,-3.13f,.5f)-3.14159265f)>.001f)return 10;
    // Angulo, inclinacao e rotacao: graus viram radianos nos campos nativos; zero nao mexe; limites e NaN.
    const auto same=AdjustViewAngles({.2f,-1,0},.1f,0,0,0);
    if(same.angles.x!=.2f || same.angles.y!=-1 || same.angles.z!=0 || same.roll!=.1f)return 13;
    const auto turned=AdjustViewAngles({0,0,0},0,90,30,-15);
    if(std::abs(turned.angles.y+1.5707963f)>.0001f || std::abs(turned.angles.x-.5235988f)>.0001f ||
       std::abs(turned.roll+.2617994f)>.0001f || turned.angles.z!=0)return 14;
    const auto clamped=AdjustViewAngles({0,0,0},0,500,-200,std::numeric_limits<float>::quiet_NaN());
    if(std::abs(clamped.angles.y+3.1415927f)>.0001f || std::abs(clamped.angles.x+1.0471976f)>.0001f || clamped.roll!=0)return 15;
    std::puts("Presentation: HUD anchors/scale, focal ratio, free camera direction and view angle offsets passed");
}
