#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <span>

namespace dev_rules {
struct MenuKeyPulse {
    uint64_t until=0;
    bool back=false;
    void Press(uint64_t now,bool menu_visible) { until=now+100;back=menu_visible; }
    bool Active(uint64_t now) const { return now<until; }
    void Clear() { until=0; }
};
struct MenuChord {
    static constexpr uint16_t mask = (1u << 6) | (1u << 7);
    bool latched = false;
    bool Update(uint16_t buttons) {
        if (!(buttons & mask)) latched = false;
        if (!latched && (buttons & mask) == mask) { latched = true; return true; }
        return false;
    }
};
// Volta ao titulo segurando Start + Back + LT + RT, como o reset do PS2
// (Start + Select + L2 + R2). O tempo evita reinicio por toque acidental.
struct TitleResetChord {
    static constexpr uint16_t mask = (1u << 4) | (1u << 5);
    static constexpr uint8_t trigger = 128;
    static constexpr double hold = 2.0;
    enum Result { Idle, Started, Fire };
    double since = -1;
    bool fired = false;
    static bool Held(uint16_t buttons, uint8_t left, uint8_t right) {
        return (buttons & mask) == mask && left >= trigger && right >= trigger;
    }
    Result Update(uint16_t buttons, uint8_t left, uint8_t right, double now) {
        if (!Held(buttons, left, right)) { since = -1; fired = false; return Idle; }
        if (fired) return Idle;
        if (since < 0) { since = now; return Started; }
        if (now - since >= hold) { fired = true; return Fire; }
        return Idle;
    }
};
struct ReleaseGate {
    bool armed = false;
    bool Filter(bool enabled, bool held) {
        if (!enabled) { armed = false;return false; }
        if (!held) armed = true;
        return armed && held;
    }
};
inline bool RelativeAsset(const std::filesystem::path& path) {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) return false;
    for (const auto& part : path)
        if (part == ".." || part == "." || part.string().find(':') != std::string::npos) return false;
    return true;
}
inline bool CheatAddress(uint32_t address, int width) {
    return (width == 1 || width == 2 || width == 4) && address >= 0x10000 &&
           uint64_t(address) + uint32_t(width) <= 0x04000000;
}
inline bool ReadOnlyGameOpen(uint32_t access, uint32_t disposition, uint32_t options) {
    constexpr uint32_t mutations = 0x520D0116u;
    return disposition == 1 && !(access & mutations) && !(options & 0x1000u);
}
inline uint32_t WidthMask(int width) {
    return width == 1 ? 0xFFu : width == 2 ? 0xFFFFu : 0xFFFFFFFFu;
}
inline float AudioSample(float value, float gain) {
    return std::isfinite(value) ? std::clamp(value * gain, -1.0f, 1.0f) : 0;
}
inline void MixAudio(std::span<float> frames, float gain, float balance, bool muted) {
    const float level = muted ? 0 : std::clamp(gain, 0.0f, 1.0f);
    const float left = level * (balance > 0 ? 1 - balance : 1);
    const float right = level * (balance < 0 ? 1 + balance : 1);
    for (size_t i = 0; i + 1 < frames.size(); i += 2) {
        frames[i] = AudioSample(frames[i], left);
        frames[i + 1] = AudioSample(frames[i + 1], right);
    }
}
// Botao digital do teclado: Start/Back e direcional (setas navegam nos menus nativos).
inline constexpr uint16_t KeyboardDigitalBit(int code) {
    if(code==8) return 1u<<4;
    if(code==9) return 1u<<5;
    if(code>=24 && code<28) return uint16_t(1u<<(code-24));
    return 0;
}
// Configuracao salva antes de bind_18 tinha Enter no Start; Enter agora confirma (A).
inline constexpr int MigratedStartKey(int saved,bool has_menu_confirm,int enter,int start_default) {
    return !has_menu_confirm && saved==enter ? start_default : saved;
}
// WM_KEYDOWN: bit 30 do lParam = tecla ja estava apertada (auto-repeat do Windows).
inline constexpr bool FirstKeyDown(uint64_t lparam) { return !(lparam & (uint64_t(1) << 30)); }
inline int16_t MoveAxis(bool positive, bool negative) {
    return positive == negative ? 0 : positive ? 32767 : -32767;
}
inline int16_t MouseAxis(int delta, float sensitivity, bool invert) {
    float value = std::clamp(float(delta) * sensitivity, -1.0f, 1.0f);
    return int16_t(value * (invert ? -32767 : 32767));
}
}
