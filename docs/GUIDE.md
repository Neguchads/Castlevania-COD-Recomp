# Player guide: Castlevania: Curse of Darkness Recomp

English · [Português (Brasil)](#pt-br)

> **Note:** this page describes the code in [PR #2](https://github.com/Neguchads/Castlevania-COD-Recomp/pull/2) (`fix/visual-hud`), which the upcoming v0.1.0 release is built from. Until it is merged, some tools, scripts and options named here exist only on that branch.

This guide explains how to install, play and configure everything the port offers. The technical side of mods (file format, texture packs, `cheats.toml`) is in [MODDING.md](../MODDING.md).

The tools menu starts in Brazilian Portuguese. Switch it with the **PT-BR** and **EN** buttons in its top-right corner (it changes immediately), or in Suporte e menu > Aparência do menu > "Idioma do menu" with the controller. The choice is saved in `settings.toml` (`menu_language`). Option names below appear as they do in the Portuguese menu, with the English menu name in parentheses.

## PC requirements

| | Minimum | Tested on |
|---|---|---|
| OS | Windows 10 or 11, 64-bit | Windows 11 Pro |
| CPU | x86-64 with SSE4.2 and POPCNT (the code is built with `-march=x86-64-v2`): Intel Core from 2008 on or AMD from 2011 on | Intel Core i5-10400F (6 cores) |
| GPU | Direct3D 12, feature level 11_0, with an up-to-date driver | NVIDIA GeForce RTX 3060, 12 GB |
| RAM | Not measured | 32 GB |
| Disk | 6.6 GB with everything: game 2.9 GB, HD4K 3.3 GB and PT-BR 0.5 GB. Without HD4K, about 3.3 GB | SSD |
| Libraries | Visual C++ Redistributable 2015–2022 **x64** | Installed |

On the test machine, the game runs at 4K (3840×2160) with 200% internal resolution and a 60 FPS cap. Occasional FPS drops are still under investigation. On a weaker PC, start with 100% internal resolution and without HD4K.

Linux: Direct3D 12 runs through Proton (vkd3d), but this was not tested.

## Install and launch

**Play folder:** in the project folder, run in PowerShell:

```powershell
.\tools\release.ps1 -Destination "G:\Jogos\Castlevania Curse of Darkness Recomp"
```

It builds, runs the tests and, only if everything passes, creates or updates the folder with the game, the mods, the saves and the settings, plus a "Castlevania Curse of Darkness Recomp" shortcut on the desktop. Use any folder on a data disk; the script remembers it, so next time `.\tools\release.ps1` is enough. Double-click the shortcut (or `CastlevaniaCOD.exe`) to play.

- **Update:** run `.\tools\release.ps1` again after changing the code. It updates the game and the mods, but does not touch the folder's saves or settings.
- **Another disk or PC:** copy the whole folder with File Explorer, which keeps the file dates. The dll must stay newer than the exe; if it is not, the game looks for the `compiler` folder and stops.
- **Start over:** delete the save folder in `hdd\E\UDATA\4b4e002d` (the one that contains `C2Save.dat`).
- **Icon:** your own `assets\icon.ico` becomes the exe and window icon on the next build.

**From the project (development):** see "Install and run" in the [README](../README.md). The exe is in `out\build\release`.

The folder contains your game files. Do not share it.

## Hotkeys

| Action | Controller | Keyboard |
|---|---|---|
| Open and close the tools menu | L3 + R3 (click both sticks) | F1 |
| Search for a menu option | Y opens the on-screen keyboard | Ctrl+F |
| Return to the title screen (unsaved progress is lost) | Hold Start + Back + LT + RT for 2 s | — |
| Capture a screenshot and graphics diagnostics | — | F3 |
| Capture and open the bug report | — | F8 or Print Screen |
| Toggle sound | — | F9 |
| Fullscreen or window | — | Alt+Enter |
| Game menu / back inside it | Start / B | Esc |

The tools menu, L3 + R3 and Start + Back + LT + RT always use the physical buttons. That is why they still work with a remapped controller.

### In-game keyboard and mouse (default, remappable)

| Command | Key |
|---|---|
| Move | WASD |
| Camera | Mouse |
| D-pad (game menus) | Arrow keys |
| A: confirm or skip / confirm in menus | Space / Enter |
| B: back or dodge | E |
| X: attack / Y: special attack | Left click / right click |
| Black / white | Q / R |
| Left / right trigger | Shift / Ctrl |
| Start / Back (map) | P / Tab |

## Tools menu

The menu opens over the game, and the game keeps running: the sticks move the character and the camera, and the buttons go to the menu. To pause everything, turn on "Pausar o jogo com o menu aberto" (pause the game while the menu is open) in Suporte e menu > Aparência do menu (menu appearance).

Navigation:
- L1/R1 switch tabs.
- The d-pad picks a title in the left column, and A enters its options.
- On a slider, A starts adjusting; the d-pad or the left stick drags it, and B stops.
- B goes back from the options to the titles. Only L3 + R3 or F1 close the menu.
- **Salvar opções** (save settings) writes everything to `settings.toml`.

**Search:** the field with the magnifier, at the top, finds any option by name, ignoring accents and word order.
- On the keyboard, type and press Enter to open the first match.
- On the controller, Y opens the on-screen keyboard, A types and B closes it. The keyboard has Apagar (backspace), Limpar (clear) and "Abrir 1o" (open first match) keys.

### Jogabilidade (gameplay)

| Page | What it has |
|---|---|
| Jogo (game) | Game language (English / Português) |
| Dificuldade (difficulty) | Presets Fácil, Normal, Difícil, Muito difícil and Pesadelo (easy to nightmare); sliders for damage taken by the player and by the Innocent Devil. The finishing blow and scripted deaths still kill. |
| Todos os inimigos (all enemies) | Enemy HP (0.5x–5x) and size (0.5x–3x); presets Original, Variados (varied), Elites, Gigantes (giants) and Caotico (chaotic); "Restaurar padrão dos inimigos" (reset enemy options) button |
| Inimigos aleatórios (random enemies) | Each enemy rolls its size and HP when it spawns, within the minimum and maximum ranges. The elite chance (0–100%) and the extra elite size and HP multiply on top. "Trocar tipos de inimigo" (swap enemy types) is still being researched. |
| Inimigos por tipo (enemies by type) | HP and size for each enemy type already seen, saved as a mod in `mods/Editor/enemies.toml`; they apply with "Aplicar edições" (apply edits) on. Includes "Voltar ao original" (reset this type) and "Voltar todos os tipos ao original" (reset all types). |
| Drops e itens (drops and items) | Item drop rate (0x–10x); random items taken from the game's own drop table; size of items on the ground (0.25x–2x, "Itens pela metade" (half-size items) button) |
| Innocent Devils | Innocent Devil crystal chance (0x–10x) and crystals per drop (1x–5x, at most 4 per drop); live diagnostic of the crystal roll. The Devil's Bestiary, Gráfico (chart) and Linha evolutiva (evolution line) pages only show up after opening the right chests (original game). |
| Câmera (camera) | Modes Original, Cinemática (cinematic), Próxima (close), Personalizada (custom), Livre (free) and Orbital. Distance, height, field of view, horizontal angle, tilt and horizon roll, plus smooth follow and free-camera speed. |
| Minimapa e HUD (minimap and HUD) | Minimap (on/off, rotate with the camera, size 0.6–1.6x, opacity, horizontal and vertical position, "top-right corner" button). HUD: Show the original HUD; HP and MP always visible; HUD scale; hide faster outside combat and the delay |
| Saves e sessão (saves and session) | Lists the save files; "Reiniciar no título" (restart at title) to load another save. The menu does not delete or replace saves. |

The enemy layers multiply: all × type × roll. Size and smaller items are visual only; the hit area and the pickup area stay the original ones. HP applies to enemies that spawn after the change, so change rooms to see it.

Every page with options has "Restaurar padrões desta página" (restore this page's defaults); the footer has "Restaurar padrões" (restore defaults) next to "Voltar ao jogo" (back to game), with a confirmation.

### Vídeo e áudio (video and audio)

| Page | What it has |
|---|---|
| Gráficos | Fullscreen or window; resolution; internal resolution; anisotropic filtering; FPS cap; vsync; presentation shader (Original, grayscale, light CRT); brightness, contrast and saturation; on-screen FPS, with a choice of corner; character shadow (off by default) |
| Áudio | Master volume; left/right balance; mute (F9); mute when the window loses focus |

### Controles (controls)

| Page | What it has |
|---|---|
| Controle (controller) | Remap each game button (press A on the button, then press the new physical button); Xbox and PS2 layouts; swap A and B; invert the right stick's horizontal camera (the game's own option only inverts the vertical axis); disable the L3 + R3 shortcut |
| Teclado e mouse (keyboard and mouse) | Remap keys; enable the keyboard; mouse as camera; invert the mouse on each axis; sensitivity; reset to default |

### Cheats, Mods and Suporte e menu (support and menu)

| Tab | What it has |
|---|---|
| Cheats | Godmode; speedhack (0.5x–4x; above 1x only 1 in N frames is shown so the game really speeds up); full HP; full MP; infinite MP; max gold; gold and EXP gain (1x–10x); purchases cost no gold; "Simular save de Lament of Innocence" (simulate a *Lament of Innocence* save, unlocks the Moai statue); custom `cheats.toml` |
| Mods | Lists the mods in `mods/`, with a checkbox for each; reload the list; export textures to build an HD pack |
| Suporte e menu | **Relatar bug** (report a bug): game frame capture, title and text, saved in `reports/`. **Capturas salvas** (saved captures): the 12 newest. **Logs e travamentos** (logs and crashes): last log lines and crash dumps. **Diagnóstico** (diagnostics): performance logging, enemy HP probe, current area and room. Nothing is sent to the internet. |
| Suporte e menu > Aparência do menu (menu appearance) | Menu language, opacity, colors, pause while the menu is open. |
| Suporte e menu > Perfis (profiles) | Save and apply sets of options, Dev profile and vanilla defaults. |

## Bundled mods

| Mod | What it does | How to enable |
|---|---|---|
| PTBR-PS2 | Brazilian Portuguese translation of 2479 texts, images and dub, taken from the fans' PS2 ISO | Options > Language > Português in the game (also in Jogabilidade > Jogo) |
| HD4K | 2739 4K textures from the PS2 pack (PlatinumMimic), matched to the Xbox ones | Mods tab |
| PS2-Botoes | ✕ ○ □ △ L1 L2 R1 R2 icons instead of the Xbox buttons | Mods tab; for the triggers, also use Controles > Layout PS2 |
| Editor | Per-type enemy HP and size | Jogabilidade > Inimigos por tipo > "Aplicar edições" |

Mods never change the original game files: they replace them in memory only. To make your own, see [MODDING.md](../MODDING.md). Credits for third-party material are in [CREDITS.md](CREDITS.md).

## What changes from the original Xbox game

- **Video:** runs on Windows with Direct3D 12, at any resolution, with higher internal resolution, anisotropic filtering and on-screen FPS. The character shadow drew in the right place after the 4X multisample fix, but it still showed up wrong in some places, so it is off by default (Gráficos > Sombra do personagem).
- **Language:** Brazilian Portuguese translation and dub, with language switching during play.
- **Textures:** 4K pack and PS2 button icons.
- **Controls:** keyboard and mouse; Xbox, PlayStation and generic controllers (SDL3); remapping; horizontal camera inversion.
- **Tools:** developer menu with search, alternative cameras and an on-screen minimap.
- **Gameplay:** damage-based difficulty; enemy HP, size, random rolls and elites; drop rate, crystals and random items; HP, MP, gold and EXP cheats (own tab); Moai bonus without the PS2 save.
- **Support:** bug report with capture and log, performance logging and crash dumps.

## Experimental features and status

"Validated in game" means someone played it and saw it work. "Tested" means automated tests or the runtime self-test passed, but nobody has confirmed it while playing yet. Report anything odd with F8.

### Works, validated in game

| Feature | Notes |
|---|---|
| Brazilian Portuguese text, images and dub | Switching language mid-game; images and voices already loaded change after a restart. |
| HD4K pack and title-screen HD | Matched textures load without FPS drops. |
| Enemy HP (all enemies) | 0.5x–5x through the game's own max-HP formula (185 → 925 at 5x). |
| Return to title (Start + Back + LT + RT) | Unsaved progress is lost. |

### Implemented and tested, waiting for in-game confirmation

| Feature | What is still unknown |
|---|---|
| Enemy size (all, per type, random, elites) | Fixed on 2026-10-26 to survive the game's own rescale; not yet seen working. Size is visual only. |
| Random enemy rolls and elites | Same as size; HP rolls use the native max-HP return. |
| Item drop rate and Innocent Devil crystals | Only the 5 drop checks of the enemy death routine are changed. |
| Random items in drops | The log shows swaps (`drops: random item 230 -> 242`), but the player has not noticed them yet. |
| Size of items on the ground | Visual only; the pickup area stays the original. |
| Menu search with the on-screen keyboard | Ctrl+F and Y; Enter or "Abrir 1o" opens the first match. |
| Tools menu in English (PT-BR / EN buttons, top right) | 402 menu strings; a headless test clicks the buttons. Some long English labels may be clipped on fixed-width buttons. |
| Godmode | Damage blocked in an isolated arena with the original routines; scripted deaths are not proven. |
| Speedhack 0.5x–2x | Clocks and timers measured; the full campaign at each speed is not. |
| *Lament of Innocence* save bonus | The bonus object is unlocked, but the room where it appears is not identified. |

### Experimental by design

| Feature | Limits |
|---|---|
| Cinemática, Próxima and Personalizada cameras | Framing profiles only; large angles skew movement and some cutscenes may be affected. |
| Livre (free) camera | No collision; the character does not move while it is on. |
| Orbital camera | Native collision tested on a single triangle; corners and tight rooms are untested. |
| HUD scale and fast hiding | Inventory and original menus are not scaled; full visual coverage pending. |
| Minimap | Shows rooms you have not explored; no door, save or teleport icons. |
| Custom `cheats.toml` profiles | Only for addresses you validated yourself; a wrong address can corrupt the game state. |

### Known problems and not available yet

- **Character shadow:** off by default because it still showed up away from the character (walls, rocks). Turn it on in Gráficos > Sombra do personagem; if it shows up wrong, send an F3 capture.
- **Occasional FPS drops:** under investigation; turn on performance logging and send a report.
- **Audio:** stereo only; Xbox DSP effects are partly reproduced.
- **Being researched:** swapping enemy types, more enemies per room and moving enemies.
- **Not available:** savestates.
- **Not tested in this version:** the @CRAZY and @TREVOR name codes and switching characters mid-campaign.

---

<a id="pt-br"></a>

# Guia do jogador: Castlevania: Curse of Darkness Recomp

[English](#player-guide-castlevania-curse-of-darkness-recomp) · Português (Brasil)

> **Nota:** esta página descreve o código do [PR #2](https://github.com/Neguchads/Castlevania-COD-Recomp/pull/2) (`fix/visual-hud`), do qual sai a release v0.1.0. Até o merge, algumas ferramentas, scripts e opções citados aqui só existem nessa branch.

Este guia explica como instalar, jogar e configurar tudo o que o port oferece. A parte técnica de mods (formato dos arquivos, packs de textura, `cheats.toml`) fica em [MODDING.md](../MODDING.md#pt-br).

O menu de ferramentas começa em português. Os botões **PT-BR** e **EN** no canto superior direito trocam na hora; no controle, use Suporte e menu > Aparência do menu > "Idioma do menu". A escolha fica salva em `settings.toml` (`menu_language`). Isso muda só o texto das ferramentas; o idioma do jogo é outra opção.

## Requisitos de PC

| | Mínimo | Testado em |
|---|---|---|
| Sistema | Windows 10 ou 11, 64 bits | Windows 11 Pro |
| Processador | x86-64 com SSE4.2 e POPCNT (o código é compilado com `-march=x86-64-v2`): Intel Core de 2008 em diante ou AMD de 2011 em diante | Intel Core i5-10400F (6 núcleos) |
| Placa de vídeo | Direct3D 12, nível 11_0, com driver atualizado | NVIDIA GeForce RTX 3060, 12 GB |
| Memória | Não medida | 32 GB |
| Disco | 6,6 GB com tudo: jogo 2,9 GB, HD4K 3,3 GB e PT-BR 0,5 GB. Sem o HD4K, cerca de 3,3 GB | SSD |
| Bibliotecas | Visual C++ Redistributable 2015–2022 **x64** | Instalado |

Na máquina de teste, o jogo roda em 4K (3840×2160) com resolução interna de 200% e limite de 60 FPS. Há quedas de FPS ocasionais ainda em investigação. Em PC mais fraco, comece com resolução interna 100% e sem o HD4K.

Linux: o Direct3D 12 roda pelo Proton (vkd3d), mas isso não foi testado.

## Instalar e abrir

**Pasta de jogar:** na pasta do projeto, rode no PowerShell:

```powershell
.\tools\release.ps1 -Destination "G:\Jogos\Castlevania Curse of Darkness Recomp"
```

Ele faz o build, roda os testes e, só se tudo passar, cria ou atualiza a pasta com o jogo, os mods, os saves e as opções, mais um atalho "Castlevania Curse of Darkness Recomp" na área de trabalho. Use qualquer pasta num disco de dados; o script guarda o destino, então da próxima vez basta `.\tools\release.ps1`. Clique no atalho (ou em `CastlevaniaCOD.exe`) para jogar.

- **Atualizar:** rode `.\tools\release.ps1` de novo depois de mudar o código. Ele atualiza o jogo e os mods, mas não mexe nos saves nem nas opções da pasta pronta.
- **Outro disco ou PC:** copie a pasta inteira pelo Explorador, que mantém as datas dos arquivos. A dll precisa continuar mais nova que o exe; se não estiver, o jogo procura a pasta `compiler` e para.
- **Começar do zero:** apague a pasta do save em `hdd\E\UDATA\4b4e002d` (a que contém `C2Save.dat`).
- **Ícone:** um `assets\icon.ico` seu vira o ícone do exe e da janela no próximo build.

**Pelo projeto (desenvolvimento):** veja "Como instalar e rodar" no [README](../README.md#pt-br). O exe fica em `out\build\release`.

A pasta tem os arquivos do seu jogo. Não compartilhe.

## Atalhos

| Ação | Controle | Teclado |
|---|---|---|
| Abrir e fechar o menu de ferramentas | L3 + R3 (clicar os dois analógicos) | F1 |
| Buscar uma opção no menu | Y abre o teclado na tela | Ctrl+F |
| Voltar ao título (perde o que não foi salvo) | Segurar Start + Back + LT + RT por 2 s | — |
| Capturar print e diagnóstico gráfico | — | F3 |
| Capturar e abrir o relato de bug | — | F8 ou Print Screen |
| Ligar e desligar o som | — | F9 |
| Tela cheia ou janela | — | Alt+Enter |
| Menu do jogo / voltar dentro dele | Start / B | Esc |

O menu de ferramentas, L3 + R3 e Start + Back + LT + RT usam sempre os botões físicos. Por isso funcionam mesmo com o controle remapeado.

### Teclado e mouse no jogo (padrão, remapeável)

| Comando | Tecla |
|---|---|
| Movimento | WASD |
| Câmera | Mouse |
| Direcional (menus do jogo) | Setas |
| A: confirmar ou pular / confirmar no menu | Espaço / Enter |
| B: voltar ou esquivar | E |
| X: ataque / Y: ataque especial | Clique esquerdo / clique direito |
| Preto / branco | Q / R |
| Gatilho esquerdo / direito | Shift / Ctrl |
| Start / Back (mapa) | P / Tab |

## Menu de ferramentas

O menu abre por cima do jogo, e o jogo continua rodando: os analógicos andam e giram a câmera, e os botões ficam com o menu. Para pausar tudo, ligue "Pausar o jogo com o menu aberto" em Suporte e menu > Aparência do menu.

Navegação:
- L1/R1 trocam de aba.
- O direcional escolhe um título na coluna da esquerda, e A entra nas opções dele.
- Numa barra, A entra no ajuste; o direcional ou o analógico esquerdo arrasta, e B sai.
- B volta das opções para os títulos. Só L3 + R3 ou F1 fecham o menu.
- **Salvar opções** grava tudo em `settings.toml`.

**Busca:** o campo com a lupa, no topo, acha qualquer opção pelo nome, sem acento e com as palavras em qualquer ordem.
- No teclado, digite e aperte Enter para abrir a primeira opção encontrada.
- No controle, Y abre o teclado na tela, A digita e B fecha. O teclado tem as teclas Apagar, Limpar e "Abrir 1o".

### Jogabilidade

| Página | O que tem |
|---|---|
| Jogo | Idioma do jogo (English / Português) |
| Dificuldade | Presets Fácil, Normal, Difícil, Muito difícil e Pesadelo; barras "Dano recebido pelo jogador" e "Dano recebido pelo Innocent Devil". O golpe final e as mortes por roteiro continuam matando. |
| Todos os inimigos | Vida dos inimigos (0,5x–5x) e tamanho (0,5x–3x); combinações prontas Original, Variados, Elites, Gigantes e Caotico; botão "Restaurar padrão dos inimigos" |
| Inimigos aleatórios | Cada inimigo sorteia tamanho e vida ao aparecer, dentro das faixas mínima e máxima. A chance de elite (0–100%) e o tamanho e a vida extras da elite multiplicam por cima. "Trocar tipos de inimigo" ainda está em pesquisa. |
| Inimigos por tipo | Vida e tamanho para cada tipo de inimigo já visto, como mod em `mods/Editor/enemies.toml`; valem com "Aplicar edições" ligado. Tem os botões "Voltar ao original" e "Voltar todos os tipos ao original". |
| Drops e itens | Taxa de drop de itens (0x–10x); itens aleatórios tirados da tabela de drops do próprio jogo; tamanho dos itens no chão (0,25x–2x, botão "Itens pela metade") |
| Innocent Devils | Chance de cristais dos Innocent Devils (0x–10x) e cristais por drop (1x–5x, no máximo 4 por drop); diagnóstico ao vivo do sorteio do cristal. Bestiário, Gráfico e Linha evolutiva do Devil só aparecem depois de abrir os baús certos (jogo original). |
| Câmera | Modos Original, Cinemática, Próxima, Personalizada, Livre e Orbital. Ajustes de distância, altura, campo de visão, ângulo horizontal, inclinação e rotação do horizonte, além de acompanhamento suave e velocidade da câmera livre. |
| Minimapa e HUD | Minimapa (ligar, girar com a câmera, tamanho 0,6–1,6x, opacidade, posição horizontal e vertical, botão "canto superior direito"). HUD: Mostrar HUD original; vida e MP sempre visíveis; escala do HUD; ocultar mais rápido fora de combate e tempo de espera |
| Saves e sessão | Lista os arquivos de save; "Reiniciar no título" para carregar outro save. O menu não apaga nem troca saves. |

As camadas de inimigos se multiplicam: todos × tipo × sorteio. Tamanho e itens menores são só visuais; a área de acerto e a de pegar o item continuam as originais. A vida vale para os inimigos que aparecem depois da mudança, então troque de sala para ver.

Cada página com opções tem "Restaurar padrões desta página"; o rodapé tem "Restaurar padrões" ao lado de "Voltar ao jogo", com confirmação.

### Vídeo e áudio

| Página | O que tem |
|---|---|
| Gráficos | Tela cheia ou janela; resolução; resolução interna; filtro anisotrópico; limite de FPS; vsync; shader de apresentação (Original, Escala de cinza, CRT leve); brilho, contraste e saturação; FPS na tela, com escolha do canto; sombra do personagem (desligada por padrão) |
| Áudio | Volume geral; balanço esquerda/direita; sem som (F9); silenciar quando a janela perde o foco |

### Controles

| Página | O que tem |
|---|---|
| Controle | Remapear cada botão do jogo (A em cima do botão e aperte o botão físico novo); Layout Xbox e Layout PS2; Trocar A e B; inverter a câmera horizontal do analógico direito (a opção do jogo só inverte a vertical); desligar o atalho L3 + R3 |
| Teclado e mouse | Remapear teclas; ligar o teclado; mouse como câmera; inverter o mouse em cada eixo; sensibilidade; restaurar o padrão |

### Cheats, Mods e Suporte e menu

| Aba | O que tem |
|---|---|
| Cheats | Godmode; speedhack (0,5x–4x; acima de 1x só 1 de cada N quadros aparece, para o jogo acelerar de verdade); vida cheia; MP cheio; MP infinito; ouro máximo; ganho de ouro e de EXP (1x–10x); compras sem gastar ouro; "Simular save de Lament of Innocence" (libera a estátua Moai); `cheats.toml` próprio |
| Mods | Lista os mods em `mods/`, com caixa para ligar cada um; recarregar a lista; exportar texturas para criar um pack HD |
| Suporte e menu | **Relatar bug:** captura do quadro do jogo, título e texto, salvo em `reports/`. **Capturas salvas:** as 12 mais novas. **Logs e travamentos:** últimas linhas do log e dumps de travamento. **Diagnóstico:** registro de desempenho, pesquisa da vida dos inimigos e área e sala atuais. Nada é enviado para a internet. |
| Suporte e menu > Aparência do menu | Idioma do menu, opacidade, cores, pausar com o menu aberto. |
| Suporte e menu > Perfis | Salvar e aplicar conjuntos de opções, perfil Dev e padrão vanilla. |

## Mods incluídos

| Mod | O que faz | Como ligar |
|---|---|---|
| PTBR-PS2 | Tradução de 2479 textos, imagens e dublagem em português, trazidas da ISO PS2 dos fãs | Opções > Idioma do jogo > Português (também em Jogabilidade > Jogo) |
| HD4K | 2739 texturas 4K do pacote PS2 (PlatinumMimic), pareadas com as do Xbox | Aba Mods |
| PS2-Botoes | Ícones ✕ ○ □ △ L1 L2 R1 R2 no lugar dos botões do Xbox | Aba Mods; para os gatilhos, use também Controles > Layout PS2 |
| Editor | Ajustes de vida e tamanho por tipo de inimigo | Jogabilidade > Inimigos por tipo > "Aplicar edições" |

Os mods nunca alteram os arquivos originais do jogo: substituem só na memória. Para criar o seu, veja [MODDING.md](../MODDING.md#pt-br). Os créditos dos materiais de terceiros estão em [CREDITS.md](CREDITS.md#pt-br).

## O que muda em relação ao Xbox original

- **Vídeo:** roda no Windows com Direct3D 12, em qualquer resolução, com resolução interna maior, filtro anisotrópico e FPS na tela. A sombra do personagem passou a sair no lugar certo com a correção do multisample 4X, mas ainda aparecia errada em alguns lugares, então vem desligada (Gráficos > Sombra do personagem).
- **Idioma:** tradução e dublagem em português, com troca de idioma durante o jogo.
- **Texturas:** pacote 4K e ícones de botão do PS2.
- **Controles:** teclado e mouse; controles de Xbox, PlayStation e genéricos (SDL3); remapeamento; inversão horizontal da câmera.
- **Ferramentas:** menu dev com busca, câmeras alternativas e minimapa na tela.
- **Jogabilidade:** dificuldade por dano; vida, tamanho, sorteio e elites de inimigos; taxa de drops, cristais e itens aleatórios; cheats de vida, MP, ouro e EXP; bônus Moai sem o save do PS2.
- **Suporte:** relato de bug com captura e log, registro de desempenho e dumps de travamento.

## Recursos experimentais e estado

"Validado no jogo" quer dizer que alguém jogou e viu funcionar. "Testado" quer dizer que os testes automáticos ou o autoteste do runtime passaram, mas ninguém confirmou jogando. Relate qualquer coisa estranha com F8.

### Funciona, validado no jogo

| Recurso | Observação |
|---|---|
| Texto, imagens e dublagem em português | Troca de idioma durante o jogo; imagens e vozes já carregadas mudam ao reiniciar. |
| Pacote HD4K e HD da tela de título | Texturas pareadas carregam sem queda de FPS. |
| Vida dos inimigos (todos) | 0,5x–5x pela fórmula de vida máxima do próprio jogo (185 → 925 com 5x). |
| Voltar ao título (Start + Back + LT + RT) | O progresso não salvo se perde. |

### Feito e testado, falta confirmar no jogo

| Recurso | O que ainda não se sabe |
|---|---|
| Tamanho dos inimigos (todos, por tipo, sorteio, elites) | Corrigido em 26/10 para sobreviver à reescala do próprio jogo; ainda não foi visto funcionando. O tamanho é só visual. |
| Sorteio e elites de inimigos | Igual ao tamanho; a vida sorteada usa o retorno nativo da vida máxima. |
| Taxa de drop e cristais dos Innocent Devils | Só as 5 checagens de drop da rotina de morte do inimigo mudam. |
| Itens aleatórios nos drops | O log mostra trocas (`drops: random item 230 -> 242`), mas o jogador ainda não percebeu. |
| Tamanho dos itens no chão | Só visual; a área de pegar continua a original. |
| Busca do menu com teclado na tela | Ctrl+F e Y; Enter ou "Abrir 1o" abre o primeiro resultado. |
| Menu de ferramentas em inglês (botões PT-BR / EN, canto superior direito) | 402 textos; um teste sem janela clica nos botões. Alguns textos longos em inglês podem ficar cortados em botões de largura fixa. |
| Godmode | Dano bloqueado numa arena isolada com as rotinas originais; mortes por roteiro não foram comprovadas. |
| Speedhack 0,5x–2x | Relógios e timers medidos; a campanha inteira em cada velocidade, não. |
| Bônus do save de *Lament of Innocence* | O objeto do bônus é liberado, mas a sala onde ele aparece não foi identificada. |

### Experimental por natureza

| Recurso | Limites |
|---|---|
| Câmeras Cinemática, Próxima e Personalizada | Só perfis de enquadramento; ângulos grandes entortam o movimento e algumas cenas podem ser afetadas. |
| Câmera Livre | Sem colisão; o personagem fica parado enquanto ela está ligada. |
| Câmera Orbital | Colisão nativa testada num único triângulo; cantos e salas apertadas não foram testados. |
| Escala do HUD e ocultação rápida | Inventário e menus originais não mudam de escala; cobertura visual completa pendente. |
| Minimapa | Mostra salas ainda não exploradas; sem ícones de porta, save ou teleporte. |
| Perfis próprios em `cheats.toml` | Só com endereços que você mesmo validou; endereço errado pode estragar o estado do jogo. |

### Problemas conhecidos e o que ainda não existe

- **Sombra do personagem:** desligada por padrão porque ainda aparecia longe do personagem (paredes, pedras). Ligue em Gráficos > Sombra do personagem; se aparecer errada, envie uma captura F3.
- **Quedas de FPS ocasionais:** em investigação; ligue o registro de desempenho e envie um relato.
- **Áudio:** só estéreo; os efeitos DSP do Xbox são reproduzidos em parte.
- **Em pesquisa:** trocar tipos de inimigo, mais inimigos por sala e mover inimigos.
- **Não existe:** savestate.
- **Não testados nesta versão:** códigos de nome @CRAZY e @TREVOR e troca de personagem no meio da campanha.
