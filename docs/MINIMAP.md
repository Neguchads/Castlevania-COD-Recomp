# Minimapa nativo

## Minimapa nativo automático (2026-10-24)

- Entregue minimapa ligado por padrão: canto superior direito,260x150px em1080p, fundo transparente, salas azuis e marcador dourado centrado no jogador. Ajustes em Imagem: visibilidade/tamanho/opacidade, persistidos em settings.toml. Usa drawlist sem capturar entradas e continua com menu fechado.
- Fonte real: arquivos stmap_*.dar via kernel::ReadGameFile, modelos3/strips36bytes com matriz própria; header55BA00 grupos/salas e transformação original3464B0. DAR offsets relativos ao entry. Geometria não recebe fator0,08; apenas cursorworld recebe. Não inventar retângulos/conexões.
- Segurança: posição cacheada player+D0/D8, ponteiroB41394, estado2 e flagsEE&18; sem chamar getter mutante na UI. Título sem jogador oculta overlay. Algumas cutscenes com jogador ativo podem mantê-lo. Geometria e arestas armazenadas em cache; leitura de arquivos não ocorre por quadro.
- Verificação: compile-local.cmd/test-local.cmd/verify-dev-runtime.ps1 passaram. Evidências logs/build-minimap.log, logs/tests-minimap.log, logs/runtime-minimap-verification.log e logs/runtime-minimap.log. Teste real dos assets cobre240modelos/39316triângulos, matriz/rotação e entradas inválidas. Runtime carregou253modelos únicos incluindo recursos comuns.
- OBSERVADO EM GAMEPLAY pelo screenshot da janela7340746: personagem no corredor, HUDHP122/122, minimapa visível superior direito com marcador e fundo transparente. Usuário já controlava o jogo; não foram enviados comandos nem carregado save pela automação. Comparação movimento/pontos entre todas as salas ainda pendente.
- Limites: mostra geometria das salas vizinhas independentemente da descoberta. Sem ícones de portas/save/teleporte, cores especiais ou indicador de andar. Algumas linhas internas do modelo ainda aparecem. Não alegar campanha/mapa inteiro validado.
- PID36428 foi deixado aberto sob controle do usuário. Não encerrar sem conferir estado e autorização. AtalhoF1/L3+R3 e controles anteriores preservados.

## Origem das estruturas

Header/mapa: generated/game/x86_recomp.101.cpp, sub345400 e345760/3457E0. Transformação da sala: sub345A50. Cursor: sub3464B0. Jogador: sub333870, sub333230 eFB8F0; posição cacheada retornada por35A8B0/3700F0. Estes caminhos foram conferidos no código recompilado e nos arquivos nativos, sem ferramenta Ghidra ao vivo (backend indisponível).

DAR: magic82734927, count+10h, entry20h+i*16. Entryu32 id/type/offset/size; recurso começa entry+offset. ModeloFFFF0003: +8matriz, +20hmeshcount,+24hmeshstart,+2Chlength. Mesh+0next/+8packet. Packet+Cvertexcount/+1Cindexcount/+2Cvertexoffset. Vertex36bytes XYZfloats; índicesstripuint16 após vertices alinhados4. Matriz rowvector, depois rotação/translação da sala.

Modelos lidos só na primeira partida; salas são reprojetadas ao trocar stage. Este código local requer os assets do próprio usuário e não constitui um pacote para distribuição.
