// Extensão local: mantém o SDK vizinho intacto.
#include "dev_runtime.h"
#include "dsound_runtime.inc"

BUFFER(SetPitch, 2) {
    // O pitch do Xbox usa oitavas de 4096 unidades sobre 48 kHz.
    const int32_t pitch = int32_t(a[1]);
    const double frequency = 48000.0 * std::exp2(double(pitch) / 4096.0);
    v.freq = uint32_t(std::clamp(frequency, 1.0, 200000.0));
    return 0;
}


BUFFER(SetRolloffFactor, 3) {
    const float factor = F32(a[1]);
    if (!std::isfinite(factor) || factor < 0) return 0x80070057u;
    v.rolloff = factor;
    return 0;
}

BUFFER(SetDopplerFactor, 3) {
    const float factor = F32(a[1]);
    if (!std::isfinite(factor) || factor < 0) return 0x80070057u;
    v.doppler = factor;
    return 0;
}

BUFFER(PlayEx, 4) {
    LOGR("dsound: PlayEx timestamp %08X:%08X flags %X", a[2], a[1], a[3]);
    if (a[1] || a[2]) {
        LOGE("dsound: scheduled PlayEx is not supported yet");
        return 0x80004001u;
    }
    v.has_gain = false;
    v.playing = true;
    v.looping = (a[3] & 1) != 0;
    v.paused = false;
    if (a[3] & 2) v.pos = 0;
    v.sync = (a[3] & 4) != 0;
    return 0;
}
