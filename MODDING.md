# Tools and mods

English · [Português (Brasil)](MODDING.pt-BR.md)

The tools menu is in Brazilian Portuguese. Option names appear below as they do in the menu, with an English translation in parentheses where it helps.

## Panel customization

The player guide, with every tab and option, is in [docs/GUIDE.md](docs/GUIDE.md). There are seven tabs at the top: Jogabilidade (gameplay: game and minimap, difficulty, enemies, drops and items, cheats, HUD and pets, camera, saves), Gráficos (graphics), Áudio (audio), Controles (controls), Mods, Suporte (support: bug report, diagnostics) and Menu (appearance, profiles). The magnifier field at the top searches any option by name: Ctrl+F on the keyboard, or Y on the controller, which opens an on-screen keyboard. L1/R1 or a click switch tabs. The d-pad picks a title in the left column; A enters its options. On a slider, A starts adjusting: the d-pad or the left stick drags it, and B stops adjusting. B goes back from the options to the titles and never closes the panel; only **L3 + R3** or **F1** close it. On the keyboard, arrows, Enter and Esc do the same. With the panel open the game keeps running: the sticks move the character and turn the camera (also in the Livre and Orbital cameras) and the buttons go to the panel; while a slider is being adjusted, the left stick only moves the slider. To pause everything (game, clock, vblank and sound), turn on "Pausar o jogo com o menu aberto" in Menu > Aparência.

Language: Jogabilidade > Jogo e minimapa > Idioma do jogo switches between English and Português (Brasil). Texts change immediately; images and voices already loaded change after a restart. Japanese is still in the game's Options > Language.

Difficulty (Jogabilidade > Dificuldade): presets Fácil, Normal, Difícil, Muito difícil and Pesadelo, or the sliders "Dano recebido pelo jogador" (damage taken by the player) and "Dano recebido pelo Innocent Devil" (damage taken by the Innocent Devil), 0.25x to 4x. The finishing blow and scripted deaths still kill. Enemy HP is under Jogabilidade > Inimigos (below).

Texture packs: file names (`<16 hex hash>.dds` or `.bmp`) are read once when the mods load or the language changes. A new file copied while the game is open only shows up after reloading the mods.

Cheats: besides godmode and speedhack, "Vida cheia" (full HP, does not revive), "MP cheio" (full MP), "MP infinito" (infinite MP, Hector only; the game has no hearts, Devil abilities spend MP), "Ouro maximo" (max gold, 9,999,999), "Ganho de ouro" and "Ganho de EXP" (gold and EXP gain, 1x to 10x) and "Compras nao gastam ouro" (purchases cost no gold). They are saved with the settings. Game name codes: @CRAZY (Crazy mode) and @TREVOR (play as Trevor), according to guides; not tested in this version.

*Lament of Innocence* save: on PS2, having the LoI save on the memory card unlocks the Moai statue. In Cheats, "Simular save de Lament of Innocence" makes the game find that save; turn it on, save the settings, and only then load a save or start a new game from the title screen (the log shows `cheat: Lament of Innocence save simulated`). The bonus is written into the save when you save. According to the code, it unlocks an object in a room (it does not go straight to the inventory); the room has not been identified yet.

Minimap: in Jogabilidade > Jogo e minimapa, "Posição horizontal/vertical do minimapa" (0 = left/top, 1 = right/bottom) and the button that returns it to the top-right corner. Salvar opções stores the position.

PlayStation buttons: turn on the **PS2-Botoes** mod (Mods) to see ✕ ○ □ △ L1 L2 R1 R2 instead of A B X Y L black R white, and in Controles > Controle press **Layout PS2** so L1/R1 become the triggers and L2/R2 black/white, as on PS2. **Layout Xbox** goes back to the default; the "Layout atual" (current layout) line shows Xbox, PS2 or custom. To rebuild the pack: `python tools\hd_pack_match.py buttons --pack <PS2 4K replacements folder>`.

Gráficos also has brightness, contrast and saturation; Áudio has "Silenciar quando a janela perde o foco" (mute when the window loses focus). The header and Diagnóstico show the area name (taken from the game's teleporters) with the map/room IDs; maps 0, 1, 7 and 12 still show only the ID. In Suporte > Diagnóstico, "Pesquisar vida dos inimigos (log)" (enemy HP probe) logs the fields of the targeted enemy that drop on each hit: turn it on, kill 2 or 3 enemies and save a report.

Mouse horizontal and vertical inversion are separate. The default was inverted compared to the previous build at the user's request. WASD moves, the arrows control the camera; both groups can be remapped. Restoring the default controls brings back the port's keys.

Jogabilidade > Inimigos: todos > "Vida dos inimigos" (enemy HP, 0.5x to 5x, key `enemy_hp`) multiplies the max HP the game computes for each enemy type. It applies to enemies that spawn after the change; change rooms to see it.

Jogabilidade > Inimigos: todos > "Tamanho dos inimigos" (enemy size, 0.5x to 3x, key `enemy_scale`) changes the size of all enemies; the per-type setting multiplies on top. Jogabilidade > Inimigos: por tipo sets HP and size per enemy type. The settings live in `mods/Editor/enemies.toml` (`[enemy.N]` with `name`, `hp` from 0.5 to 5 and `scale` from 0.5 to 3) and only apply with the Editor mod on; no game file changes. Size is visual only: the hit area stays the original one. The game rescales the enemy after it spawns; since 2026-10-26 the port reapplies the factor on that rescale. This fix has not been confirmed in game yet. HP works.

Jogabilidade > Inimigos: aleatório rolls the size and HP of each enemy when it spawns (`enemy_random`, ranges `enemy_random_scale_min/max` and `enemy_random_hp_min/max`). The elite chance (`enemy_elite_chance`, 0 to 100) multiplies on top with `enemy_elite_scale` and `enemy_elite_hp`. The layers multiply (all x type x roll), with a final limit of 0.5x to 3x for size and 0.5x to 5x for HP. The Original, Variados, Elites, Gigantes and Caotico buttons fill everything at once. Swapping one enemy type for another is still being researched.

The "Filtrar opções" (filter options) field at the top of the tools menu (Ctrl+F) finds any option by name, ignoring accents and word order; Enter opens its page.

Jogabilidade > Drops e itens: "Taxa de drop de itens" (item drop rate, `drop_rate`, 0x to 10x) multiplies the chance of each enemy item; "Chance de cristais dos Innocent Devils" (`crystal_rate`) and "Cristais por drop" (`crystal_amount`, 1x to 5x, at most 4 crystals per drop) handle the evolution crystals; "Itens aleatórios nos drops" (`random_drops`) swaps the item for another one from the game's drop table; "Tamanho dos itens no chão" (`item_scale`, 0.25x to 2x) only changes how dropped items look.

The game's "invert camera" option only inverts the vertical axis, as on the original Xbox. To invert the right stick's horizontal axis, turn on "Inverter camera horizontal (analogico direito)" in Controles > Controle (`invert_stick_x` in `settings.toml`).

Godmode now also intercepts the absolute HP setter, not only the delta: both went through the original wrappers in an isolated test, keeping healing and other characters intact. The second bypass was reproduced before the fix. A combat retest is still needed.

The panel can be moved by its title bar and resized by the bottom-right corner. The Aparência tab sets opacity, background, selection and focus. The numeric position/size controls also accept controller navigation. Restoring the default appearance resets the window and the black/red/gold palette; it does not delete saves or remaps.

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

Other pages of the Suporte tab:

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
