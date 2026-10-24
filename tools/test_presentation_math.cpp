#include "../src/common/presentation_math.h"
#include <cstdio>
#include <limits>
int main() {
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
    std::puts("Presentation: HUD anchors/scale, focal ratio and free camera direction passed");
}
