# Tools and mods

English · [Português (Brasil)](#pt-br)

The tools menu starts in Brazilian Portuguese; the **PT-BR** / **EN** buttons in its top-right corner (or Suporte e menu > Aparência do menu > "Idioma do menu") switch its language, saved as `menu_language` in `settings.toml`. Option names appear below as they do in the Portuguese menu, with an English translation in parentheses where it helps. The English text lives in `src/common/menu_strings_en.h`, keyed by the Portuguese text; `python tools/menu_strings.py` fails when a menu string has no translation or a translation changes the printf format.

## Panel customization

The player guide, with every tab and option, is in [docs/GUIDE.md](docs/GUIDE.md). There are six tabs at the top: Jogabilidade (gameplay: game, difficulty, enemies, drops and items, Innocent Devils, camera, minimap and HUD, saves), Vídeo e áudio (graphics, audio), Controles (controls), Cheats, Mods and Suporte e menu (support and menu: bug report, diagnostics, menu appearance, profiles). The magnifier field at the top searches any option by name: Ctrl+F on the keyboard, or Y on the controller, which opens an on-screen keyboard. L1/R1 or a click switch tabs. The d-pad picks a title in the left column; A enters its options. On a slider, A starts adjusting: the d-pad or the left stick drags it, and B stops adjusting. B goes back from the options to the titles and never closes the panel; only **L3 + R3** or **F1** close it. On the keyboard, arrows, Enter and Esc do the same. With the panel open the game keeps running: the sticks move the character and turn the camera (also in the Livre and Orbital cameras) and the buttons go to the panel; while a slider is being adjusted, the left stick only moves the slider. To pause everything (game, clock, vblank and sound), turn on "Pausar o jogo com o menu aberto" in Suporte e menu > Aparência do menu.

Language: Jogabilidade > Jogo > Idioma do jogo switches between English and Português (Brasil). Texts change immediately; images and voices already loaded change after a restart. Japanese is still in the game's Options > Language.

Difficulty (Jogabilidade > Dificuldade): presets Fácil, Normal, Difícil, Muito difícil and Pesadelo, or the sliders "Dano recebido pelo jogador" (damage taken by the player) and "Dano recebido pelo Innocent Devil" (damage taken by the Innocent Devil), 0.25x to 4x. The finishing blow and scripted deaths still kill. Enemy HP is under Jogabilidade > Inimigos (below).

Texture packs: file names (`<16 hex hash>.dds` or `.bmp`) are read once when the mods load or the language changes. A new file copied while the game is open only shows up after reloading the mods.

Cheats: besides godmode and speedhack, "Vida cheia" (full HP, does not revive), "MP cheio" (full MP), "MP infinito" (infinite MP, Hector only; the game has no hearts, Devil abilities spend MP), "Ouro maximo" (max gold, 9,999,999), "Ganho de ouro" and "Ganho de EXP" (gold and EXP gain, 1x to 10x) and "Compras nao gastam ouro" (purchases cost no gold). They are saved with the settings. Game name codes: @CRAZY (Crazy mode) and @TREVOR (play as Trevor), according to guides; not tested in this version.

*Lament of Innocence* save: on PS2, having the LoI save on the memory card unlocks the Moai statue. In Cheats, "Simular save de Lament of Innocence" makes the game find that save; turn it on, save the settings, and only then load a save or start a new game from the title screen (the log shows `cheat: Lament of Innocence save simulated`). The bonus is written into the save when you save. According to the code, it unlocks an object in a room (it does not go straight to the inventory); the room has not been identified yet.

Minimap: in Jogabilidade > Minimapa e HUD, "Posição horizontal/vertical do minimapa" (0 = left/top, 1 = right/bottom) and the button that returns it to the top-right corner. Salvar opções stores the position.

PlayStation buttons: turn on the **PS2-Botoes** mod (Mods) to see ✕ ○ □ △ L1 L2 R1 R2 instead of A B X Y L black R white, and in Controles > Controle press **Layout PS2** so L1/R1 become the triggers and L2/R2 black/white, as on PS2. **Layout Xbox** goes back to the default; the "Layout atual" (current layout) line shows Xbox, PS2 or custom. To rebuild the pack: `python tools\hd_pack_match.py buttons --pack <PS2 4K replacements folder>`.

Gráficos also has brightness, contrast and saturation; Áudio has "Silenciar quando a janela perde o foco" (mute when the window loses focus). The header and Diagnóstico show the area name (taken from the game's teleporters) with the map/room IDs; maps 0, 1, 7 and 12 still show only the ID. In Suporte > Diagnóstico, "Pesquisar vida dos inimigos (log)" (enemy HP probe) logs the fields of the targeted enemy that drop on each hit: turn it on, kill 2 or 3 enemies and save a report.

Mouse horizontal and vertical inversion are separate. The default was inverted compared to the previous build at the user's request. WASD moves, the arrows control the camera; both groups can be remapped. Restoring the default controls brings back the port's keys.

Jogabilidade > Todos os inimigos > "Vida dos inimigos" (enemy HP, 0.5x to 5x, key `enemy_hp`) multiplies the max HP the game computes for each enemy type. It applies to enemies that spawn after the change; change rooms to see it.

Jogabilidade > Todos os inimigos > "Tamanho dos inimigos" (enemy size, 0.5x to 3x, key `enemy_scale`) changes the size of all enemies; the per-type setting multiplies on top. Jogabilidade > Inimigos por tipo sets HP and size per enemy type. The settings live in `mods/Editor/enemies.toml` (`[enemy.N]` with `name`, `hp` from 0.5 to 5 and `scale` from 0.5 to 3) and only apply with the Editor mod on; no game file changes. Size is visual only: the hit area stays the original one. The game rescales the enemy after it spawns; since 2026-10-26 the port reapplies the factor on that rescale. This fix has not been confirmed in game yet. HP works.

Jogabilidade > Inimigos aleatórios rolls the size and HP of each enemy when it spawns (`enemy_random`, ranges `enemy_random_scale_min/max` and `enemy_random_hp_min/max`). The elite chance (`enemy_elite_chance`, 0 to 100) multiplies on top with `enemy_elite_scale` and `enemy_elite_hp`. The layers multiply (all x type x roll), with a final limit of 0.5x to 3x for size and 0.5x to 5x for HP. The Original, Variados, Elites, Gigantes and Caotico buttons fill everything at once. Swapping one enemy type for another is still being researched.

The "Filtrar opções" (filter options) field at the top of the tools menu (Ctrl+F) finds any option by name, ignoring accents and word order; Enter opens its page.

Jogabilidade > Drops e itens: "Taxa de drop de itens" (item drop rate, `drop_rate`, 0x to 10x) multiplies the chance of each enemy item; "Chance de cristais dos Innocent Devils" (`crystal_rate`) and "Cristais por drop" (`crystal_amount`, 1x to 5x, at most 4 crystals per drop) handle the evolution crystals; "Itens aleatórios nos drops" (`random_drops`) swaps the item for another one from the game's drop table; "Tamanho dos itens no chão" (`item_scale`, 0.25x to 2x) only changes how dropped items look.

The game's "invert camera" option only inverts the vertical axis, as on the original Xbox. To invert the right stick's horizontal axis, turn on "Inverter camera horizontal (analogico direito)" in Controles > Controle (`invert_stick_x` in `settings.toml`).

Godmode now also intercepts the absolute HP setter, not only the delta: both went through the original wrappers in an isolated test, keeping healing and other characters intact. The second bypass was reproduced before the fix. A combat retest is still needed.

The panel can be moved by its title bar and resized by the bottom-right corner. Suporte e menu > Aparência do menu sets opacity, background, selection and focus. The numeric position/size controls also accept controller navigation. Restoring the default appearance resets the window and the black/red/gold palette; it does not delete saves or remaps.

Salvar opções writes the preferences to settings.toml. Closing the panel also saves its geometry. The Controles tab shows the Xbox reference, the action and the key in separate columns.

Saves e sessão lists only the names and sizes of the progress files. "Reiniciar no título" asks you to confirm losing unsaved progress and uses the runtime's native restart; load a slot through the original Load Game. It does not restore or replace save files while the game runs.

Savestates, switching characters mid-campaign and new cheat presets have not been validated yet.

Open the game with the **Castlevania COD Recomp** shortcut. **F1** or **L3 + R3** opens and closes the menu, **F3** captures a screenshot and graphics diagnostics and saves an automatic report, **F8** opens the written report form and **F9** toggles sound. L3/R3 are the left/right stick clicks. Release the buttons before repeating the shortcut. To return to the title screen and load another save, hold **Start + Back + LT + RT** for 2 seconds (the game restarts; unsaved progress is lost).

The menu uses black, deep red and gold, Windows fonts rendered at a higher resolution and automatic scaling. On the controller, up/down navigates, left/right or L1/R1 switches sections, A/cross adjusts or confirms and B/circle stops adjusting or goes back. The shortcut that opens the menu can be disabled in Controles.

If B/Circle is already held when the menu opens, it only works as Back after you release it; this avoids closing the tools by accident.

The menu keeps the world running. The sticks control the character and camera; d-pad/A/B/L1/R1 operate the panel. Mouse and keyboard stay in the interface and do not trigger game attacks or camera. Pause the game itself before opening the tools during combat. Click **Salvar opções** to keep controls, audio, video and mods in `settings.toml`.

All paths below sit next to `CastlevaniaCOD.exe`, usually in `out/build/release`.

F3 saves the image and the report after the graphics capture finishes. The confirmation only appears on screen after the files are written. The image also goes to `reports/`; the full trace goes to `captures/orange-marker/` by default. `XBE_CAPTURE_DIR` picks another folder.

## Reporting problems

1. Press Print Screen or F8 when the problem shows up. The panel opens in Suporte > Relatar bug with the captured image.
2. Write a title and a description.
3. Click "Salvar relato local" (save local report).

Each `reports/date-time-id` folder contains `screenshot.bmp`, `report.txt` and `runtime.log`. The capture comes from the game frame, without the tools menu or the desktop. No file is sent automatically. The log matches the moment you save the report. Captures during loading may record a black frame; use "Nova captura" (new capture) when the image you want is visible.

Other pages of the Suporte e menu tab:

- **Capturas salvas** (saved captures): the 12 newest. Move the d-pad through the list to see the image; "Abrir pasta" (open folder) and "Copiar relato" (copy report) act on the selected one.
- **Logs e travamentos** (logs and crashes): the last 200 lines of this session's log ("Copiar log", "Abrir pasta do log"). Each crash writes `CrashDumps/<date>.dmp` and a copy of the log with the same name; open the item to see the last lines.

On Windows 11, if "Use the Print screen key to open screen capture" is on, the Snipping Tool may open too. Turn the option off in Settings > Accessibility > Keyboard or use F8.

## Controller remapping

In F1 > Controles > Controle, select a game button, press A (or click) and press the physical button that should trigger it. The button that already used that physical button gets the old one, so no command disappears. Esc cancels; with no answer in 6 s, the capture gives up. "Trocar A e B" (swap A and B) and "Restaurar padrão" (restore default) do what they say. Salvar opções stores the map (key `controller_map`), and saved profiles carry it along.

The tools menu, L3 + R3 and Start + Back + LT + RT always use the physical buttons. Remapping applies only to player 1 and only to buttons; the sticks do not change.

## Keyboard and mouse

| Command | Default |
|---|---|
| Move | WASD |
| Camera | Mouse, simulating the right stick |
| D-pad (navigates the game menus) | Arrow keys |
| Open the game menu / back inside it | Esc |
| A, confirm in menus | Enter |
| A, confirm/skip | Space |
| B, back/dodge | E |
| X, attack | Left click |
| Y, special attack | Right click |
| Black / white | Q / R |
| Left / right triggers | Shift / Ctrl |
| Start / Back | P / Tab |

In the Controles tab, click an action's key and press the new key or mouse button. F1/F8/F9/Esc are reserved. Settings saved before this version with Enter on Start move Start to P. The camera keeps the original game stick's limits and inversions. F1 and losing focus release the cursor. The physical controller stays available; when the mouse moves, it controls the camera.

## File mods

Create `mods/ModName/files/` and mirror the relative path of the files in `assets/`. Example: an original file `assets/DATA/example.bin` is replaced by `mods/ModName/files/DATA/example.bin`.

Open Mods, reload the list, tick the mod and save the settings. Restart the game to swap files that are already loaded. The replacement file is opened read-only. The layer only serves reads from the game disc; saves in `hdd/` stay outside it. Do not change the original assets.

If two mods provide the same file, the name that sorts last alphabetically wins. Unticking and restarting removes the effect. Absolute paths, `..` and links that leave the mod folder are rejected.

## Texture packs

1. Tick "Exportar texturas" (export textures) in the Mods tab and go through the screen you want to change.
2. The first 200 compatible textures of the run appear in `texture-dumps/`.
3. Edit the BMP keeping its 16-hex-digit name.
4. Put it in `mods/MyPack/textures/0123456789ABCDEF.bmp`.
5. Reload the list, enable the pack and save. Restart if the content was already cached.

Uncompressed 24- or 32-bit BMP up to 16384×16384 is accepted. `<hash>.dds` in BC7_UNORM (with or without mipmaps) is also accepted and uses 1/4 of the video memory; when both exist, the DDS wins. Textures in `mods/PTBR-PS2/textures` only apply with the Portuguese language, even with the packs turned off.

PS2 4K pack (PlatinumMimic, see [CREDITS.md](docs/CREDITS.md)): `python tools/hd_pack_match.py --pack <replacements folder>` matches the images with the Xbox textures; `... build --pack <folder>` generates `mods/HD4K/textures` as DDS BC7 (needs `.local/build-tools/texconv.exe`); `... near --pack <folder>` uses the near-identical ones (4K detail where the image matches, upscaled original where it differs; `--dry-run --preview <folder>` to check first); `... ptbr --pack <folder>` builds the translated textures in high resolution. Then enable `HD4K` in F1 > Mods. Use 32 bits to keep the alpha channel of transparent textures. The identifier depends on the original content, format, dimensions and palette, not on the temporary memory address. Cube, volume and render-target textures are not replaced. Export covers formats the backend converts to BGRA8. Packs accept 24/32-bit BMP and DDS BC7 with mipmaps; PNG is not read by the game (convert it with the tool).

The `PTBR-PS2` mod has no checkbox of its own: it turns on with **Options → Language → Português (Brasil)** in the game and does not turn off other mods.
See [TRANSLATION.md](docs/TRANSLATION.md) for local setup and coverage limits.

Anisotropic filtering and internal resolution are in Gráficos. They improve sampling; HD detail needs files from a pack. The available shaders are Original, Escala de cinza (grayscale) and CRT leve (light CRT), applied to the final presentation.

## Xbox cheats

**Godmode** is in Jogabilidade > Cheats. It protects the main character's HP by blocking negative deltas in the original HP routine of this Xbox USA version. Healing and other actors keep working. The program checks the routine's signature before enabling the option. Godmode starts off on every run and shows how many hits it blocked.

The implementation reads the game's ID map to protect the player slot. The logical ID can be 0 or 1; it is not the physical position in the list. The test uses the original routines that build this map and apply damage in a separate arena: normal damage 100→90, Godmode 100→100, healing 100→110 and another actor 100→90, including when ID 0 belongs to another slot. It does not remove hit animations or revive a character who is already dead. Deaths caused by scripts or other paths have not been proven in gameplay.

For extra profiles, copy `cheats.example.toml` to `cheats.toml` and fill in only addresses you have validated for this project's Xbox USA version. PS2/GameShark codes cannot be pasted directly.

Each entry needs `name`, `address`, `width` (1, 2 or 4 bytes), `expected` and `value`. The address is relative to Xbox memory. The loader accepts data between `0x10000` and `0x04000000`; it requires a writable page and the expected value before the first write. The SHA-256 header must match the USA `default.xbe`.

Cheats start off. When on, they repeat the write every frame. When turned off, they restore the original value only if they still find the value the patch wrote. This does not prove that an address really holds HP or money. A wrong profile can change the game state; test with a copy of your save.

## Speed and limits

**Speedhack**, in Jogabilidade > Cheats, goes from 0.5× to 2×. It scales RDTSC/QPC, Xbox system time, waits, timers, VBlank, presentation and audio sample consumption. Changing speed keeps the clock continuous. Vsync is suspended outside 1× and the saved preference applies again at 1×.

Sound pitch changes with speed; pitch is not preserved. The XAudio2 output stays stereo 48 kHz. The game always starts at 1×. Use Normal to restore it.

In-runtime tests confirmed that 60 virtual ms took about 61 real ms at 1×, 31 ms at 2× and 122 ms at 0.5×. An already armed timer also followed the factor change. This does not prove the full campaign, all physics or every cutscene at each factor. Native surround, full DSP and extra profiles are still pending.

## References

The option layout is inspired by the official [PCSX2](https://github.com/PCSX2/pcsx2) and [xemu](https://github.com/xemu-project/xemu) projects. Local implementation on top of the MIT XboxXBERecomp SDK, using the ImGui and toml++ already present. No emulator code was included.

The HP routine was identified with the [XBMC4Gamers Game Trainers](https://www.xbmc4gamers.co.uk/xbox/urldownloader/other/game%20trainers.zip) package, entry T0150.etm, and checked against the bytes of the local USA XBE. No trainer was installed or run; the hook is this project's own implementation in the recompilation.


## Minimap

On by default during play, in the top-right corner. Transparent background, blue rooms and a gold marker centered on the player. It uses the stmap meshes and the original cursor transform, without opening Back or writing to the save.

In Jogo: on/off, size 0.6–1.6× and opacity 0.2–1. Salvar opções keeps the preferences for the next launch. The orientation is fixed; the outlines move with the character. It may show neighboring rooms not explored yet. Discovery and special save/teleport icons or colors do not exist yet. Some scenes with an active player may keep showing the map.


## HUD, original menu and camera

In Jogo → game options: minimap, "Mostrar HUD original" (show original HUD), scale 0.35–1.0× (default 0.5×), 50% button, fast hiding and a 0.2–5 s delay. The delay replaces the original routine's 5 s timeout; the original 0.3 s fade and its triggers remain. It is not a new enemy detector.

F1 or L3+R3 opens and closes the tools panel. Esc sends Start to the game, and B when the modal/pause manager is open, to leave or go back in the original menu. Esc does not toggle the tools panel.

Camera: Original; Cinemática, inspired by Lords of Shadow 1; Próxima, inspired by Lords of Shadow 2; Personalizada; Livre. The profiles approximate the framing and do not reproduce those games' systems. Personalizada sets distance, height and relative field of view. Personalizada and Livre also have horizontal angle (±180°), tilt (±60°) and horizon roll (±45°), added to the game's own angles; "Zerar" (reset) goes back to normal. In Personalizada, large horizontal angles leave the stick in the old reference frame.

On-screen FPS: F1 > Gráficos > "Mostrar FPS na tela" (choose the corner). Green from 55, yellow from 30 to 55, red below. FPS drops: in F1 > Suporte > Diagnóstico, turn on "Registrar desempenho no log" (log performance), play the slow part for 1 minute and save a report; the log with the worst frame and the slow frames goes with it.

Free camera: close the panel; WASD moves, Q/E go down/up and the mouse looks around. Controller: the left stick moves, the right one looks and LT/RT go down/up. Speed is in the menu. While it is active in a game, the controls stop moving the character; the world keeps running. On the title screen and character select, the native controls stay available. Switching back to Original restores the native system.

The cameras are experimental: the free camera has no collision and cutscenes are not fully detected. Some scenes may have different framing while the mode is on. Known maps and the debug camera are excluded. All native parameters changed temporarily are restored after the frame matrix is built.


## Update: restored menus and experimental progress

Native menus stay at 100% after a report of misalignment at 40%. The reduction and the world in the background were postponed on request; the 50% HUD is independent and stays adjustable. There is no active menu scale control in this revision.

In Jogo: "Girar minimapa conforme câmera" (rotate minimap with the camera) toggles rotation. Câmera → Orbital third person turns on free look and smooth follow, with the native collision query. Collision was tested on an isolated triangle; corners, areas and relative movement need gameplay testing. Original restores the native camera.

Português (Brasil): choose it in the game's **Options → Language**. It turns on the catalog of 2479 texts from the translated PS2 ISO and the `PTBR-PS2` mod (images and dub). Details in [TRANSLATION.md](docs/TRANSLATION.md).

---

<a id="pt-br"></a>

# Ferramentas e mods

[English](#tools-and-mods) · Português (Brasil)

## Personalização do painel

Idioma do menu: os botões **PT-BR** e **EN** no canto superior direito (ou Suporte e menu > Aparência do menu > "Idioma do menu", no controle) trocam o texto das ferramentas, salvo como `menu_language` em `settings.toml`. O inglês fica em `src/common/menu_strings_en.h`, com o texto em português como chave; `python tools/menu_strings.py` falha se faltar tradução ou se uma tradução mudar o formato do printf.

O guia do jogador, com cada aba e opção, está em [docs/GUIDE.md](docs/GUIDE.md#pt-br). Seis abas no topo: Jogabilidade (jogo, dificuldade, inimigos, drops e itens, Innocent Devils, câmera, minimapa e HUD, saves), Vídeo e áudio (gráficos, áudio), Controles, Cheats, Mods e Suporte e menu (relatar bug, diagnóstico, aparência do menu, perfis). O campo com a lupa, no topo, busca qualquer opção pelo nome: Ctrl+F no teclado ou Y no controle, que abre um teclado na tela. L1/R1 ou clique trocam de aba. O direcional escolhe um título na coluna esquerda; A entra nas opções dele. Numa barra, A entra no ajuste: direcional ou analógico esquerdo arrasta, B sai do ajuste. B volta das opções para os títulos e nunca fecha o painel; só **L3 + R3** ou **F1** fecham. No teclado: setas, Enter e Esc fazem o mesmo. Com o painel aberto o jogo continua rodando: os analógicos andam e giram a câmera (também na Livre e na Orbital) e os botões ficam com o painel; enquanto uma barra é ajustada, o analógico esquerdo só mexe a barra. Para pausar tudo (jogo, relógio, vblank e som), ligue "Pausar o jogo com o menu aberto" em Suporte e menu > Aparência do menu.

Idioma: Jogabilidade > Jogo > Idioma do jogo troca entre English e Português (Brasil). Textos mudam na hora; imagens e vozes já carregadas, ao reiniciar. Japonês continua em Opções > Idioma no jogo.

Dificuldade (Jogabilidade > Dificuldade): presets Fácil, Normal, Difícil, Muito difícil e Pesadelo, ou as barras "Dano recebido pelo jogador" e "Dano recebido pelo Innocent Devil" (0,25x a 4x). O golpe final e mortes por roteiro continuam matando. A vida dos inimigos fica em Jogabilidade > Inimigos (abaixo).

Pacotes de textura: os nomes dos arquivos (`<hash de 16 hex>.dds` ou `.bmp`) são lidos uma vez ao carregar os mods ou trocar o idioma. Arquivo novo copiado com o jogo aberto só aparece depois de recarregar os mods.

Cheats: além de godmode e speedhack, "Vida cheia" (não revive), "MP cheio", "MP infinito" (só o Hector; o jogo não tem corações, as habilidades dos Devils gastam MP), "Ouro maximo" (9.999.999), "Ganho de ouro" e "Ganho de EXP" (1x a 10x) e "Compras nao gastam ouro". Ficam salvos nas opções. Códigos de nome do jogo: @CRAZY (modo Crazy) e @TREVOR (jogar com Trevor), segundo guias; não testados nesta versão.

Save de Lament of Innocence: no PS2, ter o save de LoI no memory card libera a estátua Moai. Em Cheats, "Simular save de Lament of Innocence" faz o jogo achar esse save; ligue, salve as opções e só então carregue o save ou comece um jogo novo pela tela de título (o log mostra `cheat: Lament of Innocence save simulated`). O bônus fica gravado no save ao salvar. Pelo código, ele libera um objeto numa sala (não cai direto no inventário); a sala ainda não foi identificada.

Minimapa: em Jogabilidade > Minimapa e HUD, "Posição horizontal/vertical do minimapa" (0 = esquerda/topo, 1 = direita/base) e o botão que volta ao canto superior direito. Salvar opções grava a posição.

Botões de PlayStation: ligue o mod **PS2-Botoes** (Mods) para ver ✕ ○ □ △ L1 L2 R1 R2 no lugar de A B X Y L preto R branco, e em Controles > Controle aperte **Layout PS2** para L1/R1 virarem os gatilhos e L2/R2 o preto/branco, como no PS2. **Layout Xbox** volta ao padrão; a linha "Layout atual" mostra Xbox, PS2 ou personalizado. Para gerar o pack de novo: `python tools\hd_pack_match.py buttons --pack <pasta replacements do 4K PS2>`.

Gráficos também têm brilho, contraste e saturação; Áudio tem "Silenciar quando a janela perde o foco". O cabeçalho e o Diagnóstico mostram o nome da área (tirado dos teleportes do jogo) com os IDs de mapa/sala; os mapas 0, 1, 7 e 12 ainda aparecem só com o ID. Em Suporte > Diagnóstico, "Pesquisar vida dos inimigos (log)" registra os campos do inimigo alvo que caem a cada golpe: ligue, mate 2 ou 3 inimigos e salve um relato.

Mouse horizontal/vertical têm inversão separada. O padrão foi invertido em relação ao build anterior por pedido do usuário. WASD movimenta, setas controlam câmera; os dois grupos podem ser remapeados. Restaurar controles padrão repõe as teclas do port.

Jogabilidade > Todos os inimigos > "Vida dos inimigos" (0,5x a 5x, chave `enemy_hp`) multiplica a vida máxima calculada pelo jogo para cada tipo de inimigo. Vale para inimigos que aparecem depois da mudança; troque de sala para ver.

Jogabilidade > Todos os inimigos > "Tamanho dos inimigos" (0,5x a 3x, chave `enemy_scale`) muda o tamanho de todos; o ajuste por tipo multiplica por cima. Jogabilidade > Inimigos por tipo ajusta vida e tamanho por tipo de inimigo. Os ajustes ficam em `mods/Editor/enemies.toml` (`[enemy.N]` com `name`, `hp` de 0,5 a 5 e `scale` de 0,5 a 3) e só valem com o mod Editor ligado; nenhum arquivo do jogo muda. O tamanho é só visual: a área de acerto continua a original. O jogo reescala o inimigo depois que ele aparece; desde 26/10 o port reaplica o fator nessa reescala. Essa correção ainda não foi confirmada no jogo. A vida funciona.

Jogabilidade > Inimigos aleatórios sorteia tamanho e vida de cada inimigo ao aparecer (`enemy_random`, faixas `enemy_random_scale_min/max` e `enemy_random_hp_min/max`). A chance de elite (`enemy_elite_chance`, 0 a 100) multiplica por cima com `enemy_elite_scale` e `enemy_elite_hp`. As camadas se multiplicam (todos x tipo x sorteio), com limite final de 0,5x a 3x no tamanho e 0,5x a 5x na vida. Os botões Original, Variados, Elites, Gigantes e Caotico preenchem tudo de uma vez. Trocar um tipo de inimigo por outro ainda está em pesquisa.

O campo "Filtrar opções" no topo do menu dev (Ctrl+F) acha qualquer opção pelo nome, sem acento e em qualquer ordem de palavras; Enter abre a página dela.

Jogabilidade > Drops e itens: "Taxa de drop de itens" (`drop_rate`, 0x a 10x) multiplica a chance de cada item do inimigo; "Chance de cristais dos Innocent Devils" (`crystal_rate`) e "Cristais por drop" (`crystal_amount`, 1x a 5x, no máximo 4 cristais por drop) cuidam dos cristais de evolução; "Itens aleatórios nos drops" (`random_drops`) troca o item por outro da tabela de drops do jogo; "Tamanho dos itens no chão" (`item_scale`, 0,25x a 2x) muda só o visual dos itens que caem.

A opção "inverter câmera" das Opções do jogo só inverte o eixo vertical, como no Xbox original. Para inverter o horizontal do analógico direito, ligue "Inverter camera horizontal (analogico direito)" em Controles > Controle (`invert_stick_x` em `settings.toml`).

Godmode agora intercepta também o setter absoluto de HP, além do delta: ambos passaram pelos wrappers originais em teste isolado, preservando cura e outros personagens. O segundo bypass foi reproduzido antes da correção. Reteste em combate continua necessário.

O painel pode ser movido pela barra de título e redimensionado pelo canto inferior direito. Suporte e menu > Aparência do menu ajusta opacidade, fundo, seleção e foco. Os controles numéricos de posição/tamanho também aceitam navegação por controle. Restaurar aparência padrão repõe a janela e a paleta preto/vermelho/dourado; não apaga saves nem remapeamentos.

Salvar opções grava as preferências em settings.toml. Fechar o painel grava também sua geometria. A aba Controles mostra a referência Xbox, a ação e a tecla em colunas separadas.

Saves e sessão lista apenas nomes/tamanhos dos arquivos de progresso. Reiniciar no título pede confirmação da perda de progresso não salvo e usa o reinício nativo do runtime; carregue um slot pelo Load Game original. Não restaura nem substitui arquivos de save enquanto o jogo roda.

Savestate, troca de personagem no meio da campanha e novos presets de cheats ainda não foram validados.

Abra o jogo pelo atalho **Castlevania COD Recomp**. **F1** ou **L3 + R3** abre/fecha o menu, **F3** captura print e diagnóstico gráfico e salva um relatório automático, **F8** abre o formulário de relato escrito e **F9** liga/desliga o som. L3/R3 são os cliques nos analógicos esquerdo/direito. Solte os botões antes de repetir o atalho. Para voltar ao título e carregar outro save, segure **Start + Back + LT + RT** por 2 segundos (o jogo reinicia; progresso não salvo é perdido).

O menu usa preto, vermelho profundo e dourado, fontes do Windows renderizadas em resolução maior e escala automática. No controle, cima/baixo navega, esquerda/direita ou L1/R1 troca de seção, A/cruz ajusta/confirma e B/círculo sai do ajuste ou volta. A combinação para abrir pode ser desativada em Controles.

Se B/Círculo já estiver segurado ao abrir, ele só passa a funcionar como Voltar depois que você o soltar; isso evita fechar as ferramentas sem querer.

O menu mantém o mundo rodando. Analógicos controlam personagem e câmera; direcional/A/B/L1/R1 operam o painel. Mouse e teclado ficam na interface e não acionam ataques/câmera do jogo. Pause pelo próprio jogo antes de abrir as ferramentas durante combate. Clique em **Salvar opções** para persistir controles, áudio, vídeo e mods em `settings.toml`.

Todos os caminhos abaixo ficam ao lado de `CastlevaniaCOD.exe`, normalmente em `out/build/release`.

F3 salva a imagem e o relatório depois que a captura gráfica termina. A confirmação aparece na tela só após os arquivos serem gravados. A imagem também vai para `reports/`; o rastreio completo fica em `captures/orange-marker/` por padrão. `XBE_CAPTURE_DIR` permite escolher outra pasta.

## Relatar problemas

1. Pressione Print Screen ou F8 quando o problema aparecer. O painel abre em Suporte > Relatar bug com a imagem capturada.
2. Escreva título e descrição.
3. Clique em Salvar relato local.

Cada pasta `reports/data-hora-identificador` contém `screenshot.bmp`, `report.txt` e `runtime.log`. A captura vem do quadro do jogo, sem o menu de ferramentas nem a Área de Trabalho. Nenhum arquivo é enviado automaticamente. O log corresponde ao momento em que você salva o relato. Capturas durante carregamento podem registrar um quadro preto; use Nova captura quando a imagem desejada estiver visível.

Outras páginas da aba Suporte e menu:

- **Capturas salvas**: as 12 mais novas. Passe o direcional pela lista para ver a imagem; Abrir pasta e Copiar relato agem sobre a selecionada.
- **Logs e travamentos**: últimas 200 linhas do log desta sessão (Copiar log, Abrir pasta do log). Cada travamento grava `CrashDumps/<data>.dmp` e uma cópia do log com o mesmo nome; abra o item para ver as últimas linhas.

No Windows 11, se a opção "Usar a tecla Print Screen para abrir a captura de tela" estiver ligada, a Ferramenta de Captura pode abrir junto. Desligue a opção em Configurações > Acessibilidade > Teclado ou use F8.

## Mapeamento do controle

Em F1 > Controles > Controle, selecione um botão do jogo, aperte A (ou clique) e pressione o botão físico que deve acioná-lo. O botão que já usava esse físico fica com o antigo, então nenhum comando some. Esc cancela; sem resposta em 6 s, a captura desiste. "Trocar A e B" e "Restaurar padrão" fazem o que dizem. Salvar opções grava o mapa (chave `controller_map`), e os perfis salvos o levam junto.

O menu dev, L3 + R3 e Start + Back + LT + RT usam sempre os botões físicos. Só vale para o jogador 1 e só para botões; os analógicos não mudam.

## Teclado e mouse

| Comando | Padrão |
|---|---|
| Movimento | WASD |
| Câmera | Mouse, simulando o analógico direito |
| Direcional (navega nos menus do jogo) | Setas |
| Abrir menu do jogo / voltar dentro dele | Esc |
| A, confirmar no menu | Enter |
| A, confirmar/pular | Espaço |
| B, voltar/esquivar | E |
| X, ataque | Clique esquerdo |
| Y, ataque especial | Clique direito |
| Preto / branco | Q / R |
| Gatilhos esquerdo / direito | Shift / Ctrl |
| Start / Back | P / Tab |

Na aba Controles, clique na tecla de uma ação e pressione a nova tecla ou botão do mouse. F1/F8/F9/Esc são reservados. Configuração salva antes desta versão com Enter no Start passa o Start para P. A câmera conserva as limitações e inversões do analógico original do jogo. F1 e a perda de foco liberam o cursor. O controle físico continua disponível; quando houver movimento de mouse, ele controla a câmera.

## Mods de arquivos

Crie `mods/NomeDoMod/files/` e reproduza o caminho relativo dos arquivos em `assets/`. Exemplo: um arquivo original `assets/DATA/exemplo.bin` recebe a substituição `mods/NomeDoMod/files/DATA/exemplo.bin`.

Abra Mods, recarregue a lista, marque o mod e salve as opções. Reinicie o jogo para trocar arquivos já carregados. O arquivo substituto é aberto somente para leitura. A camada só atende leituras do disco do jogo; saves em `hdd/` ficam fora dela. Não altere os assets originais.

Se dois mods oferecerem o mesmo arquivo, o nome em ordem alfabética mais alta vence. Desmarcar e reiniciar remove o efeito. Caminhos absolutos, `..` e saídas da pasta do mod por links são rejeitados.

## Packs de texturas

1. Marque Exportar texturas na aba Mods e passe pela tela que deseja alterar.
2. As primeiras 200 texturas compatíveis da execução aparecem em `texture-dumps/`.
3. Edite o BMP preservando seu nome de 16 dígitos hexadecimais.
4. Coloque-o em `mods/MeuPack/textures/0123456789ABCDEF.bmp`.
5. Recarregue a lista, ative o pack e salve. Reinicie se o conteúdo já estava em cache.

Aceita BMP sem compressão, 24 ou 32 bits, até 16384×16384. Também aceita `<hash>.dds` em BC7_UNORM (com ou sem mipmaps), que ocupa 1/4 da memória de vídeo; quando os dois existem, o DDS vence. Texturas em `mods/PTBR-PS2/textures` valem só com o idioma Português, mesmo com os packs desligados.

Pacote 4K do PS2 (PlatinumMimic, ver [CREDITS.md](docs/CREDITS.md#pt-br)): `python tools/hd_pack_match.py --pack <pasta replacements>` pareia as imagens com as texturas do Xbox; `... build --pack <pasta>` gera `mods/HD4K/textures` em DDS BC7 (precisa de `.local/build-tools/texconv.exe`); `... near --pack <pasta>` aproveita as quase iguais (detalhe do 4K onde a imagem bate, original ampliada onde difere; `--dry-run --preview <pasta>` para conferir antes); `... ptbr --pack <pasta>` monta as texturas traduzidas em alta. Depois ligue `HD4K` em F1 > Mods. Use 32 bits para manter o canal alpha das transparências. O identificador depende do conteúdo original, formato, dimensões e paleta, não do endereço temporário na memória. Texturas de cubo, volume e render targets ficam fora da substituição. A exportação atende formatos convertidos pelo backend para BGRA8. Packs aceitam BMP 24/32 bits e DDS BC7 com mipmaps; PNG não é lido pelo jogo (converta com a ferramenta).

O mod `PTBR-PS2` não tem checkbox próprio: ele liga com **Opções → Idioma →
Português (Brasil)** no jogo e não desliga outros mods.
Veja [TRANSLATION.md](docs/TRANSLATION.md#pt-br) para montagem local e limites de cobertura.

Filtro anisotrópico e resolução interna estão em Gráficos. Eles melhoram a amostragem; detalhes HD exigem arquivos de um pack. Os shaders disponíveis são Original, Escala de cinza e CRT leve, aplicados à apresentação final.

## Cheats de Xbox

**Godmode** está em Jogabilidade > Cheats. Ele protege a vida do personagem principal, bloqueando deltas negativos na rotina original de HP desta versão Xbox USA. A cura e os outros atores continuam funcionando. O programa confere a assinatura da rotina antes de habilitar a opção. Godmode começa desligado a cada execução e mostra quantos danos bloqueou.

A implementação consulta o mapa de IDs do jogo para proteger o slot do jogador. O ID lógico pode ser 0 ou 1; não é a posição física na lista. O teste usa as rotinas originais de construção desse mapa e de aplicação de dano em uma arena separada: dano normal 100→90, Godmode 100→100, cura 100→110 e outro ator 100→90, inclusive quando o ID 0 pertence a outro slot. Não remove animações de impacto nem revive personagem já morto. Mortes comandadas por scripts ou outros caminhos não foram comprovadas em gameplay.

Para perfis adicionais, copie `cheats.example.toml` para `cheats.toml` e preencha apenas endereços que você validou para o Xbox USA deste projeto. Códigos de PS2/GameShark não podem ser colados diretamente.

Cada entrada precisa de `name`, `address`, `width` (1, 2 ou 4 bytes), `expected` e `value`. O endereço é relativo à memória Xbox. O carregador aceita dados entre `0x10000` e `0x04000000`; exige uma página gravável e o valor esperado antes da primeira aplicação. O cabeçalho SHA-256 precisa corresponder ao `default.xbe` USA.

Cheats começam desligados. Ativados, repetem a escrita a cada quadro. Ao desligar, restauram o valor original somente se ainda encontrarem o valor escrito pelo patch. Isso não valida que um endereço represente vida ou dinheiro. Um perfil errado pode alterar o estado do jogo; teste com cópia do seu save.

## Velocidade e limites

**Speedhack**, em Jogabilidade > Cheats, varia de 0,5× a 2×. Agora escala RDTSC/QPC, tempo do sistema Xbox, esperas, timers, VBlank, apresentação e consumo das amostras de áudio. A troca de velocidade mantém o relógio contínuo. Vsync é temporariamente suspenso fora de 1× e a preferência salva volta a valer em 1×.

O som muda de tom junto com a velocidade; não há preservação de pitch. A saída XAudio2 permanece estéreo 48 kHz. O jogo sempre inicia em 1×. Use Normal para restaurar.

Testes dentro do runtime confirmaram que 60 ms virtuais levaram aproximadamente 61 ms reais em 1×, 31 ms em 2× e 122 ms em 0,5×. Um timer já armado também respondeu à mudança de fator. Isso não comprova campanha inteira, toda a física ou todas as cenas de vídeo em cada fator. Surround nativo, DSP completo e perfis adicionais continuam pendentes.

## Referências

Organização das opções inspirada nos projetos oficiais [PCSX2](https://github.com/PCSX2/pcsx2) e [xemu](https://github.com/xemu-project/xemu). Implementação local sobre o SDK XboxXBERecomp MIT, usando ImGui e toml++ já presentes. Nenhum código dos emuladores foi incorporado.

Identificação da rotina de HP conferida com o pacote [Game Trainers do XBMC4Gamers](https://www.xbmc4gamers.co.uk/xbox/urldownloader/other/game%20trainers.zip), entrada T0150.etm, e com os bytes do XBE USA local. Nenhum trainer foi instalado ou executado; o hook é implementação própria na recompilação.


## Minimapa

Ligado por padrão durante a partida, no canto superior direito. Fundo transparente, salas azuis e marcador dourado centrado no jogador. Usa malhas stmap e transformação do cursor original, sem abrir Back nem escrever no save.

Em Jogo: ligar/desligar, tamanho0,6–1,6× e opacidade0,2–1. Salvar opções mantém preferências no próximo início. Orientação fixa; os contornos se deslocam com o personagem. Pode mostrar salas vizinhas ainda não exploradas. Descoberta e ícones/cores especiais de save/teleporte ainda não existem. Algumas cenas com jogador ativo podem continuar mostrando o mapa.


## HUD, menu original e câmera

Em Jogo → Opções de jogo: minimapa, Mostrar HUD original, escala0,35–1,0× (padrão0,5×), botão50%, ocultação rápida e espera0,2–5s. A espera substitui o prazo de5s da rotina original; o fade original0,3s e seus gatilhos permanecem. Não é um detector novo de inimigos.

F1 ou L3+R3 abre/fecha o painel dev. Esc envia Start ao jogo e B quando o gerenciador modal/pausa está aberto, para sair/voltar no menu original. Esc não alterna o painel dev.

Câmera: Original; Cinemática inspirada em Lords of Shadow1; Próxima inspirada em Lords of Shadow2; Personalizada; Livre. Os perfis aproximam enquadramento e não reproduzem os sistemas desses jogos. Personalizada ajusta distância, altura e campo de visão relativo. Personalizada e Livre têm ainda ângulo horizontal (±180°), inclinação (±60°) e rotação do horizonte (±45°), somados aos ângulos do próprio jogo; "Zerar" volta ao normal. Na Personalizada, ângulos horizontais grandes deixam o analógico no referencial antigo.

FPS na tela: F1 > Gráficos > Mostrar FPS na tela (escolha o canto). Verde a partir de 55, amarelo de 30 a 55, vermelho abaixo. Quedas de FPS: em F1 > Suporte > Diagnóstico, ligue "Registrar desempenho no log", jogue o trecho lento por 1 minuto e salve um relato; o log com pior quadro e quadros lentos vai junto.

Câmera livre: fechar painel, WASD move, Q/E desce/sobe e mouse olha. Controle: analógico esquerdo move, direito olha e LT/RT descem/sobem. Velocidade no menu. Enquanto ativa na partida, os comandos deixam de mover o personagem; o mundo continua rodando. No título e na seleção de personagem os comandos nativos permanecem disponíveis. Voltar a Original restaura o sistema nativo.

Câmeras são experimentais: não há colisão da câmera livre nem identificação completa de cutscenes. Algumas cenas podem ter enquadramento alterado enquanto o modo estiver ativo. Mapas e câmera debug identificados ficam excluídos. Todos os parâmetros nativos alterados temporariamente são restaurados após gerar a matriz do quadro.


## Atualização: menus restaurados e progresso experimental

Menus nativos permanecem100% após relato de desalinhamento em40%. A redução e o mundoao fundo foram adiados a pedido; HUD50% é independente e continua ajustável. Não há controle ativo de escala dos menus nesta revisão.

Em Jogo: Girar minimapa conforme câmera liga/desliga a rotação. Câmera → Orbital em terceira pessoa ativa free look e acompanhamento suave, com consulta nativa de colisão. Colisão foi testada em triângulo isolado; cantos, áreas e movimento relativo precisam teste emgameplay. Original restaura câmera nativa.

Português (Brasil): escolha em **Opções → Idioma** no jogo. Liga o catálogo de 2479 textos da ISO PS2 traduzida e o mod `PTBR-PS2` (imagens e dublagem). Detalhes em [TRANSLATION.md](docs/TRANSLATION.md#pt-br).
