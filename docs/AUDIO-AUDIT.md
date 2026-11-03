# Audio check, 2026-10-23

English · [Português (Brasil)](#pt-br)

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

---

<a id="pt-br"></a>

# Verificação de áudio, 2026-10-23

[English](#audio-check-2026-10-23) · Português (Brasil)

## Evidência real
- Serviços Audiosrv e AudioEndpointBuilder ativos; dispositivos de som consultados com status OK.
- O mesmo src/common/xaudio2_output.cpp ligado ao jogo foi compilado no teste tools/probe-audio.cmd.
- XAudio2 inicializou sem erro. Entrada: float32, 2 canais, 48000 Hz.
- Endpoint padrão: 8 canais, 48000 Hz, máscara 0x63F (7.1 com laterais e traseiras).
- O teste silencioso executou 201 callbacks em dois segundos e enviou os buffers sem erro. Isso verifica o fluxo de saída, não a audição física nos fones.
- O jogo passou a iniciar o XAudio2 após tratar SetPitch, SetRolloffFactor, SetDopplerFactor e PlayEx imediato.

## Canais
- Saída atual do port é estéreo. O XAudio2 aceita essa fonte mesmo com endpoint 7.1.
- Centro e traseiros são reduzidos para os canais esquerdo/direito; LFE não é enviado separadamente, conforme o comportamento herdado do SDK.
- Teste do código de produção: separação esquerda/direita, centro simétrico, traseiros, exclusão de LFE e saturação passaram em tools/test-local.cmd.
- Não foi configurado surround nativo, nem alterado o dispositivo padrão do Windows ou o Wave Link.

## Limites e achados
- Não há evidência de que a configuração de canais tenha causado os acessos inválidos. Esses acessos vinham de funções originais DirectSound lendo estruturas de Xbox em objetos nativos.
- O SDK não reproduz todos os efeitos DSP; o próprio jogo registra efeitos não reproduzidos.
- PlayEx implementado para timestamp zero (observado no jogo). Agendamento com timestamp diferente de zero retorna E_NOTIMPL com erro explícito, em vez de alegar suporte.
- Algumas APIs herdadas do SDK continuam aproximadas ou vazias. Não há validação de fidelidade sonora completa nem garantia de terminar o jogo.
- Com a inicialização avançando sem o fechamento anterior, a tela permanece preta. Diagnóstico gráfico em andamento.

## Como repetir
- tools/test-local.cmd
- tools/probe-audio.cmd
- compile-local.cmd
- Evidência da saída: logs/audio-output-probe.log.
