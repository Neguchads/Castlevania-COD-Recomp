# Native minimap

English · [Português (Brasil)](#pt-br)

## Automatic native minimap (2026-10-24)

- Delivered a minimap on by default: top-right corner, 260x150 px at 1080p, transparent background, blue rooms and a gold marker centered on the player. Settings in Imagem: visibility/size/opacity, saved in settings.toml. It uses a draw list without capturing input and keeps working with the menu closed.
- Real source: stmap_*.dar files through kernel::ReadGameFile, models 3/strips of 36 bytes with their own matrix; header 55BA00 for groups/rooms and the original transform 3464B0. DAR offsets are relative to the entry. The geometry does not get the 0.08 factor; only the cursor world position does. Do not invent rectangles or connections.
- Safety: cached position at player+D0/D8, pointer B41394, state 2 and flags EE&18; no mutating getter is called from the UI. The title screen without a player hides the overlay. Some cutscenes with an active player may keep it. Geometry and edges are cached; files are not read every frame.
- Verification: compile-local.cmd, test-local.cmd and verify-dev-runtime.ps1 passed. Evidence: logs/build-minimap.log, logs/tests-minimap.log, logs/runtime-minimap-verification.log and logs/runtime-minimap.log. The real-asset test covers 240 models/39316 triangles, matrix/rotation and invalid input. The runtime loaded 253 unique models, including common resources.
- SEEN IN GAMEPLAY in a window screenshot: character in a corridor, HUD HP 122/122, minimap visible at the top right with the marker and a transparent background. The user was already controlling the game; the automation sent no commands and loaded no save. Comparing movement and points across all rooms is still pending.
- Limits: shows the geometry of neighboring rooms regardless of discovery. No door/save/teleport icons, special colors or floor indicator. Some inner model lines still show. Do not claim the whole campaign or map is validated.
- The game was left open under the user's control. Do not close it without checking the state and getting permission. The F1/L3+R3 shortcut and the earlier controls are preserved.

## Where the structures come from

Header/map: generated/game/x86_recomp.101.cpp, sub 345400 and 345760/3457E0. Room transform: sub 345A50. Cursor: sub 3464B0. Player: sub 333870, sub 333230 and FB8F0; cached position returned by 35A8B0/3700F0. These paths were checked in the recompiled code and in the native files, without live Ghidra (backend unavailable).

DAR: magic 82734927, count at +10h, entry at 20h+i*16. Entry u32 id/type/offset/size; the resource starts at entry+offset. Model FFFF0003: +8 matrix, +20h mesh count, +24h mesh start, +2Ch length. Mesh +0 next/+8 packet. Packet +C vertex count/+1C index count/+2C vertex offset. Vertex of 36 bytes with XYZ floats; uint16 strip indices after the vertices, aligned to 4. Row-vector matrix, then the room rotation/translation.

Models are read only in the first game session; rooms are reprojected when the stage changes. This local code needs the user's own assets and is not a package for distribution.

---

<a id="pt-br"></a>

# Minimapa nativo

[English](#native-minimap) · Português (Brasil)

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
