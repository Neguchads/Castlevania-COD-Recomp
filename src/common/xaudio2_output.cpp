// mixer output through XAudio2
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <xaudio2.h>

#include <algorithm>
#include <cmath>
#include <thread>
#include <vector>

#include "audio/audio.h"
#include "dev_runtime.h"
#include "core/log.h"

namespace audio {

void StartOutput(std::function<void(float*, uint32_t)> render) {
    std::thread([render] {
        CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        IXAudio2* xa = nullptr;
        IXAudio2MasteringVoice* master = nullptr;
        IXAudio2SourceVoice* voice = nullptr;
        WAVEFORMATEX wf{};
        wf.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        wf.nChannels = 2;
        wf.nSamplesPerSec = kRate;
        wf.wBitsPerSample = 32;
        wf.nBlockAlign = 8;
        wf.nAvgBytesPerSec = kRate * 8;
        HRESULT hr = XAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (SUCCEEDED(hr)) hr = xa->CreateMasteringVoice(&master);
        if (SUCCEEDED(hr)) hr = xa->CreateSourceVoice(&voice, &wf);
        if (FAILED(hr)) {
            LOGE("audio: XAudio2 unavailable (%08X), no sound", unsigned(hr));
            return;
        }
                XAUDIO2_VOICE_DETAILS master_details{};
        master->GetVoiceDetails(&master_details);
        DWORD channel_mask = 0;
        HRESULT mask_hr = master->GetChannelMask(&channel_mask);
        LOGR("audio: XAudio2 ready: source %u channels, %u Hz, float32; endpoint %u channels, %u Hz; mask %08X (hr %08X)",
             unsigned(wf.nChannels), unsigned(wf.nSamplesPerSec), unsigned(master_details.InputChannels),
             unsigned(master_details.InputSampleRate), unsigned(channel_mask), unsigned(mask_hr));
        hr = voice->Start();
        if (FAILED(hr)) {
            LOGE("audio: Start failed (%08X)", unsigned(hr));
            voice->DestroyVoice(); master->DestroyVoice(); xa->Release(); CoUninitialize();
            return;
        }
        constexpr uint32_t kFrames = kRate / 100;
        constexpr uint32_t kMinQueued = 3;
        constexpr uint32_t kMaxQueued = 10;
        // one slot more than can be queued, so the one rendered is never one XAudio2 still reads
        constexpr uint32_t kSlots = kMaxQueued + 1;
        std::vector<float> bufs(kSlots * kFrames * 2);
        uint32_t depth = kMinQueued;
        bool reported_signal = false;
        for (uint32_t n = 0;;) {
            XAUDIO2_VOICE_STATE st;
            voice->GetState(&st, XAUDIO2_VOICE_NOSAMPLESPLAYED);
            // a starved voice crackles: queue one buffer more each time, up to kMaxQueued
            if (n && !st.BuffersQueued && depth < kMaxQueued) {
                depth++;
                LOGI("audio: output starved, queueing %u ms", depth * 10);
            }
            if (st.BuffersQueued >= depth) {
                Sleep(2);
                continue;
            }
            float* b = &bufs[(n % kSlots) * kFrames * 2];
            // pausado pelo menu dev: silencio sem consumir o audio do jogo, que continua de onde parou
            if (dev::GamePaused()) std::fill(b, b + kFrames * 2, 0.0f);
            else render(b, kFrames);
            dev::ApplyAudio(b, kFrames);
            if (!reported_signal) {
                float left_peak = 0, right_peak = 0;
                for (uint32_t frame = 0; frame < kFrames; frame++) {
                    left_peak = std::max(left_peak, std::fabs(b[frame * 2]));
                    right_peak = std::max(right_peak, std::fabs(b[frame * 2 + 1]));
                }
                if (left_peak > .00001f || right_peak > .00001f) {
                    LOGR("audio: nonzero stereo signal: left peak %.5f, right peak %.5f", left_peak, right_peak);
                    reported_signal = true;
                }
            }
            XAUDIO2_BUFFER xb{};
            xb.AudioBytes = kFrames * 8;
            xb.pAudioData = reinterpret_cast<const BYTE*>(b);
                        hr = voice->SubmitSourceBuffer(&xb);
            if (FAILED(hr)) {
                LOGE("audio: SubmitSourceBuffer failed (%08X)", unsigned(hr));
                voice->DestroyVoice(); master->DestroyVoice(); xa->Release(); CoUninitialize();
                return;
            }
            if (n == 20) LOGR("audio: XAudio2 streaming: submitted 21 blocks; queued %u", unsigned(st.BuffersQueued));
            n++;  // only a submitted buffer takes a slot, XAudio2 still reads the queued ones
        }
    }).detach();
}

}  // namespace audio
