#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace dev {
inline float MenuContentScale(float width,float height) {
    if(!std::isfinite(width) || !std::isfinite(height) || width<=0 || height<=0) return 1;
    return std::clamp(std::min(width/1120.0f,height/760.0f),.25f,8.0f);
}
struct MenuPreferences {
    float x=40,y=40,width=1120,height=760,opacity=.88f;
    std::array<float,3> background{.065f,.043f,.048f};
    std::array<float,3> accent{.75f,.19f,.25f};
    std::array<float,3> highlight{.88f,.76f,.50f};
    bool positioned=false;
    void Sanitize() {
        auto finite=[](float value,float fallback){return std::isfinite(value) ? value : fallback;};
        x=std::clamp(finite(x,40),0.0f,16384.0f);y=std::clamp(finite(y,40),0.0f,16384.0f);
        width=std::clamp(finite(width,1120),640.0f,7680.0f);
        height=std::clamp(finite(height,760),420.0f,4320.0f);
        opacity=std::clamp(finite(opacity,.88f),.25f,1.0f);
        for(auto* color : {&background,&accent,&highlight})
            for(float& channel : *color) channel=std::clamp(finite(channel,.2f),0.0f,1.0f);
    }
};
inline MenuPreferences menu_preferences;
inline bool menu_geometry_reset=false;
}
