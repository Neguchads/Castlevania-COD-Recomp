#include "dev_runtime.h"
#include "scaled_clock.h"
#include "core/log.h"
#include <chrono>
#include <mutex>

namespace dev {
namespace {
int64_t HostNow() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
struct ClockState { std::mutex lock; ScaledClock clock{HostNow()}; };
ClockState& State() { static ClockState state; return state; }
}
uint64_t VirtualNanoseconds() {
    auto& state = State();
    std::lock_guard guard(state.lock);
    return uint64_t(state.clock.Now(HostNow()));
}
void SetSpeed(float factor) {
    auto& state = State();
    std::lock_guard guard(state.lock);
    state.clock.SetRate(HostNow(), factor);
    speed.store(float(state.clock.Rate()));
    LOGR("dev: guest speed %.2fx (clock, waits, vblank, audio)", speed.load());
}
void PauseClock(bool paused) {
    auto& state = State();
    std::lock_guard guard(state.lock);
    state.clock.SetPaused(HostNow(), paused);
    LOGR("dev: guest clock %s", paused ? "paused by dev menu" : "resumed");
}
int64_t RealWaitNanoseconds(int64_t virtual_ns) {
    const long double value = std::max(0.0L,static_cast<long double>(virtual_ns) / speed.load());
    return value >= static_cast<long double>(INT64_MAX) ? INT64_MAX : int64_t(value);
}
}
