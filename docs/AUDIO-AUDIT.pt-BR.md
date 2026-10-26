# Verificação de áudio, 2026-10-23

[English](AUDIO-AUDIT.md) · Português (Brasil)

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
