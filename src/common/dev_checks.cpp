#include "dev_runtime.h"
#include "kernel/kernel.h"
#include "core/log.h"
#include "cpu/x86_context.h"
#include <chrono>
#include <thread>

namespace dev {
bool VerifySpeedRuntime() {
    using Clock=std::chrono::steady_clock;
    bool ok=true;
    for(const float factor : {1.0f,2.0f,.5f}) {
        const auto prior=VirtualNanoseconds();
        SetSpeed(factor);
        const bool monotonic=VirtualNanoseconds()>=prior;
        const auto start=Clock::now();
        const uint64_t cycles=x86_rdtsc();
        const int64_t system=kernel::Now100ns();
        const uint32_t result=kernel::Wait({},false,-600000,false);
        const double real=std::chrono::duration<double>(Clock::now()-start).count();
        const double qpc=double(x86_rdtsc()-cycles)/733333333.0;
        const double elapsed=double(kernel::Now100ns()-system)/10000000.0;
        const double expected=.06/double(factor);
        const bool clocks=std::abs(qpc-elapsed)<.015 && qpc>=.055 && qpc<.3;
        const bool waited=result==kernel::kTimeout && real>expected*.6 && real<expected*2+.03;
        auto timer=std::make_shared<kernel::Object>(kernel::ObjType::Timer);
        timer->notification=true;
        kernel::SetTimer(timer,-400000,0,0);
        const auto timer_start=Clock::now();
        const bool signaled=kernel::Wait({timer},false,-4000000,false)==kernel::kWait0;
        const double timer_real=std::chrono::duration<double>(Clock::now()-timer_start).count();
        kernel::CancelTimer(*timer);
        const bool timer_ok=signaled && timer_real>.04/factor*.5 && timer_real<.04/factor*2+.03;
        LOGR("dev selftest: speed %.1fx clocks=%d wait=%d timer=%d real_ms=%.1f",factor,clocks,waited,timer_ok,real*1000);
        ok=ok && monotonic && clocks && waited && timer_ok;
    }
    SetSpeed(1);
    auto timer=std::make_shared<kernel::Object>(kernel::ObjType::Timer);
    timer->notification=true;
    const auto start=Clock::now();
    kernel::SetTimer(timer,-1200000,0,0);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    SetSpeed(2);
    bool changed=kernel::Wait({timer},false,-10000000,false)==kernel::kWait0;
    const double real=std::chrono::duration<double>(Clock::now()-start).count();
    changed=changed && real>.04 && real<.115;
    kernel::CancelTimer(*timer);
    SetSpeed(1);
    LOGR("dev selftest: speed runtime=%d armed-timer rate-change=%d",ok,changed);
    return ok && changed;
}
}
