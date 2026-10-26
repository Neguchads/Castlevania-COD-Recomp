#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace cod_view {
inline float HudCoordinate(float value,float scale,bool horizontal) {
    const float anchor=horizontal ? -256.0f : (value>0 ? 224.0f : -224.0f);
    return anchor+(value-anchor)*scale;
}
// Painel do inimigo (sub_00309010): nome e barra terminam em x=216 (0xD8), "Lv" em 240.
// O ponto 216 vai para o centro da tela; a altura segue a ancora inferior do HUD.
inline float EnemyPanelCoordinate(float value,float scale) {
    return (value-216.0f)*scale;
}
inline int32_t Focal(int32_t value,float field_scale) {
    if(value<=0 || !std::isfinite(field_scale) || field_scale<=0) return value;
    return int32_t(std::clamp(std::round(value/field_scale),64.0f,8192.0f));
}
struct Vec3 { float x=0,y=0,z=0; };
inline Vec3 Direction(float yaw,float pitch) {
    return {std::sin(yaw)*std::cos(pitch),std::sin(pitch),std::cos(yaw)*std::cos(pitch)};
}
inline Vec3 OrbitTarget(Vec3 player,float height) {
    return {player.x,player.y-120-height,player.z};
}
inline Vec3 OrbitPosition(Vec3 target,float yaw,float pitch,float distance) {
    const auto forward=Direction(yaw,pitch);
    return {target.x-forward.x*distance,target.y-forward.y*distance,target.z-forward.z*distance};
}
inline float FollowYaw(float current,float wanted,float factor) {
    return current+std::remainder(wanted-current,6.28318530718f)*std::clamp(factor,0.f,1.f);
}
// Ajuste do usuario (graus) sobre os angulos nativos da view, em radianos: sub_00361FE0 grava
// view+0x20 = (inclinacao, -angulo, 0); sub_00362190 gira a matriz em Z por view+0x15C (sub_00386900).
inline constexpr float kMaxCameraYaw=180,kMaxCameraPitch=60,kMaxCameraRoll=45;
struct ViewAngles { Vec3 angles; float roll=0; };
inline ViewAngles AdjustViewAngles(Vec3 angles,float roll,float yaw_degrees,float pitch_degrees,float roll_degrees) {
    constexpr float radians=3.14159265358979f/180;
    auto limit=[](float value,float maximum){return std::isfinite(value) ? std::clamp(value,-maximum,maximum) : 0.0f;};
    return {{angles.x+limit(pitch_degrees,kMaxCameraPitch)*radians,angles.y-limit(yaw_degrees,kMaxCameraYaw)*radians,angles.z},
            roll+limit(roll_degrees,kMaxCameraRoll)*radians};
}
}
