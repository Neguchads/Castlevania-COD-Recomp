# Audio check, 2026-10-23

English · [Português (Brasil)](AUDIO-AUDIT.pt-BR.md)

## Real evidence
- The Audiosrv and AudioEndpointBuilder services are running; the sound devices queried report status OK.
- The same src/common/xaudio2_output.cpp linked into the game was compiled in the tools/probe-audio.cmd test.
- XAudio2 started without errors. Input: float32, 2 channels, 48000 Hz.
- Default endpoint: 8 channels, 48000 Hz, mask 0x63F (7.1 with side and rear channels).
- The silent test ran 201 callbacks in two seconds and submitted the buffers without errors. This checks the output path, not what is physically heard on headphones.
- The game now starts XAudio2 after handling SetPitch, SetRolloffFactor, SetDopplerFactor and immediate PlayEx.

## Channels
- The port's current output is stereo. XAudio2 accepts this source even with a 7.1 endpoint.
- Center and rear channels are folded into left/right; LFE is not sent separately, following the behavior inherited from the SDK.
- Production code test: left/right separation, symmetric center, rears, LFE exclusion and saturation passed in tools/test-local.cmd.
- Native surround was not set up, and neither the Windows default device nor Wave Link was changed.

## Limits and findings
- There is no evidence that the channel setup caused the invalid accesses. Those accesses came from original DirectSound functions reading Xbox structures in native objects.
- The SDK does not reproduce every DSP effect; the game itself logs the effects that are not reproduced.
- PlayEx is implemented for a zero timestamp (seen in the game). Scheduling with a non-zero timestamp returns E_NOTIMPL with an explicit error, instead of claiming support.
- Some APIs inherited from the SDK are still approximate or empty. There is no full sound fidelity validation and no guarantee that the game can be finished.
- With startup moving forward without the earlier shutdown, the screen stayed black. Graphics diagnosis in progress at the time.

## How to repeat
- tools/test-local.cmd
- tools/probe-audio.cmd
- compile-local.cmd
- Output evidence: logs/audio-output-probe.log.
