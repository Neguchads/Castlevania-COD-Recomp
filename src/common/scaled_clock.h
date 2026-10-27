#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace dev {
// faixa do speedhack (menu, settings.toml e relogio)
inline constexpr double kMinSpeed = .5, kMaxSpeed = 4;
class ScaledClock {
    int64_t real_anchor_ = 0, virtual_anchor_ = 0;
    double rate_ = 1;
    bool paused_ = false;
public:
    explicit ScaledClock(int64_t real_now) : real_anchor_(real_now) {}
    int64_t Now(int64_t real_now) const {
        if (paused_) return virtual_anchor_;
        const long double elapsed = std::max<int64_t>(0, real_now - real_anchor_);
        const long double value = virtual_anchor_ + elapsed * rate_;
        return value >= static_cast<long double>(INT64_MAX) ? INT64_MAX : int64_t(value);
    }
    void SetRate(int64_t real_now, double rate) {
        virtual_anchor_ = Now(real_now);
        real_anchor_ = std::max(real_anchor_, real_now);
        rate_ = std::isfinite(rate) ? std::clamp(rate, kMinSpeed, kMaxSpeed) : 1;
    }
    // Menu dev aberto: o tempo do jogo para; ao voltar, continua de onde parou.
    void SetPaused(int64_t real_now, bool paused) {
        if (paused == paused_) return;
        virtual_anchor_ = Now(real_now);
        real_anchor_ = std::max(real_anchor_, real_now);
        paused_ = paused;
    }
    double Rate() const { return rate_; }
};
inline int64_t GuestDeadline(int64_t due_100ns, int64_t now_ns, int64_t now_100ns) {
    const long double delay = due_100ns < 0 ? -static_cast<long double>(due_100ns) * 100 :
        std::max(static_cast<long double>(due_100ns) - now_100ns, 0.0L) * 100;
    const long double deadline = static_cast<long double>(now_ns) + delay;
    return deadline >= static_cast<long double>(INT64_MAX) ? INT64_MAX : int64_t(deadline);
}
}
