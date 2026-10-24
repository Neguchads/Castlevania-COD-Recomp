#include "audio/audio.h"
#include "core/log.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

std::atomic<unsigned> calls{0};
int main() {
    logx::Init("logs", "audio-output-probe");
    audio::StartOutput([](float* out, uint32_t frames) {
        std::fill(out, out + frames * 2, 0.0f);
        calls.fetch_add(1);
    });
    std::this_thread::sleep_for(std::chrono::seconds(2));
    const unsigned count = calls.load();
    LOGR("probe: render callbacks %u; silent test, no speaker tone", count);
    logx::Flush();
    std::printf("Callbacks: %u\n", count);
    std::fflush(stdout);
    std::_Exit(count >= 100 ? 0 : 1);
}
