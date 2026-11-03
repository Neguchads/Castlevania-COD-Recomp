#include "../src/common/audio_math.h"
#include <cstdio>

int main() {
    using namespace cod_audio;
    float bins[6] = {};
    bins[0] = .5f;
    if (DownmixStereo(bins).left != .5f || DownmixStereo(bins).right != 0) return 10;
    bins[0] = 0; bins[1] = .5f;
    if (DownmixStereo(bins).left != 0 || DownmixStereo(bins).right != .5f) return 11;
    bins[1] = 0; bins[2] = .5f;
    if (DownmixStereo(bins).left != DownmixStereo(bins).right || DownmixStereo(bins).left <= 0) return 12;
    bins[2] = 0; bins[3] = 1;
    if (DownmixStereo(bins).left != 0 || DownmixStereo(bins).right != 0) return 13;
    bins[3] = 0; bins[4] = .5f;
    if (DownmixStereo(bins).left <= 0 || DownmixStereo(bins).right != 0) return 14;
    bins[4] = 0; bins[5] = .5f;
    if (DownmixStereo(bins).left != 0 || DownmixStereo(bins).right <= 0) return 15;
    for (float& bin : bins) bin = 1;
    if (DownmixStereo(bins).left != 1 || DownmixStereo(bins).right != 1) return 16;
    if (DopplerRatio(1, 0, 0, 1) != 1) return 1;
    if (DopplerRatio(1, 0, -30, 1) <= 1) return 2;
    if (DopplerRatio(1, 0, 30, 1) >= 1) return 3;
    if (DopplerRatio(1, 0, 30, 0) != 1) return 4;
    if (!std::isfinite(DopplerRatio(1, 10000, -10000, 1))) return 5;
    if (Attenuation(5, 10, 100, 1) != 1) return 6;
    if (std::abs(Attenuation(20, 10, 100, 1) - .5f) > .0001f) return 7;
    if (Attenuation(20, 10, 100, 0) != 1) return 8;
    if (Attenuation(20, 10, 100, 2) >= .5f) return 9;
    std::puts("Audio: canais, downmix, Doppler e atenuacao passaram");
}
