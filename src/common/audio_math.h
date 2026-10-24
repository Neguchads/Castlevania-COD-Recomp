#pragma once
#include <algorithm>
#include <cmath>

namespace cod_audio {
struct StereoFrame { float left, right; };
inline StereoFrame DownmixStereo(const float* bins) {
    constexpr float fold = .707f;
    return {std::clamp(bins[0] + fold * (bins[2] + bins[4]), -1.0f, 1.0f),
            std::clamp(bins[1] + fold * (bins[2] + bins[5]), -1.0f, 1.0f)};
}
inline double DopplerRatio(float distance, float listener_speed, float source_speed, float factor) {
    if (distance <= 0 || factor == 0) return 1;
    constexpr float sound_speed = 343.3f;
    const float listener = std::clamp(listener_speed * factor, -sound_speed * .99f, sound_speed * .99f);
    const float source = std::clamp(source_speed * factor, -sound_speed * .99f, sound_speed * .99f);
    return std::clamp(double(sound_speed + listener) / double(sound_speed + source), .125, 8.0);
}
inline float Attenuation(float distance, float minimum, float maximum, float rolloff) {
    if (distance <= minimum || rolloff == 0) return 1;
    return minimum / (minimum + rolloff * (std::min(distance, maximum) - minimum));
}
}
