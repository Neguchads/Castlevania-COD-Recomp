# Player guide: Castlevania: Curse of Darkness Recomp

English · [Português (Brasil)](GUIDE.pt-BR.md)

This guide explains how to install, play and configure everything the port offers. The technical side of mods (file format, texture packs, `cheats.toml`) is in [MODDING.md](../MODDING.md).

The tools menu is in Brazilian Portuguese. Option names below appear as they do in the menu, with an English translation in parentheses.

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

**Ready-to-play folder:** in the project folder, run in PowerShell:

```powershell
.\tools\make-portable.ps1
```

It creates "Castlevania Curse of Darkness Recomp" on the desktop, with the game, the mods, the saves and the settings. Double-click `CastlevaniaCOD.exe` to play.

- **Update:** run the script again after a build. It updates the game and the mods, but does not touch the folder's saves or settings.
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

The menu opens over the game, and the game keeps running: the sticks move the character and the camera, and the buttons go to the menu. To pause everything, turn on "Pausar o jogo com o menu aberto" (pause the game while the menu is open) in Menu > Aparência (appearance).

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
| Jogo e minimapa (game and minimap) | Game language (English / Português); minimap (on/off, rotate with the camera, size 0.6–1.6x, opacity, horizontal and vertical position, "top-right corner" button) |
| Dificuldade (difficulty) | Presets Fácil, Normal, Difícil, Muito difícil and Pesadelo (easy to nightmare); sliders for damage taken by the player and by the Innocent Devil. The finishing blow and scripted deaths still kill. |
| Inimigos: todos (enemies: all) | Enemy HP (0.5x–5x) and size (0.5x–3x); presets Original, Variados (varied), Elites, Gigantes (giants) and Caotico (chaotic); "Restaurar padrão dos inimigos" (reset enemy options) button |
| Inimigos: aleatório (enemies: random) | Each enemy rolls its size and HP when it spawns, within the minimum and maximum ranges. The elite chance (0–100%) and the extra elite size and HP multiply on top. "Trocar tipos de inimigo" (swap enemy types) is still being researched. |
| Inimigos: por tipo (enemies: per type) | HP and size for each enemy type already seen, saved as a mod in `mods/Editor/enemies.toml`; they apply with "Aplicar edições" (apply edits) on. Includes "Voltar ao original" (reset this type) and "Voltar todos os tipos ao original" (reset all types). |
| Drops e itens (drops and items) | Item drop rate (0x–10x); Innocent Devil crystal chance (0x–10x) and crystals per drop (1x–5x, at most 4 per drop); random items taken from the game's own drop table; size of items on the ground (0.25x–2x, "Itens pela metade" (half-size items) button) |
| Cheats | Godmode; speedhack (0.5x–2x); full HP; full MP; infinite MP; max gold; gold and EXP gain (1x–10x); purchases cost no gold; "Simular save de Lament of Innocence" (simulate a *Lament of Innocence* save, unlocks the Moai statue); custom `cheats.toml` |
| HUD e pets | Show the original HUD; HP and MP always visible; HUD scale; hide faster outside combat and the delay |
| Câmera (camera) | Modes Original, Cinemática (cinematic), Próxima (close), Personalizada (custom), Livre (free) and Orbital. Distance, height, field of view, horizontal angle, tilt and horizon roll, plus smooth follow and free-camera speed. |
| Saves e sessão (saves and session) | Lists the save files; "Reiniciar no título" (restart at title) to load another save. The menu does not delete or replace saves. |

The enemy layers multiply: all × type × roll. Size and smaller items are visual only; the hit area and the pickup area stay the original ones. HP applies to enemies that spawn after the change, so change rooms to see it.

### Gráficos (graphics), Áudio (audio) and Controles (controls)

| Tab | What it has |
|---|---|
| Gráficos | Fullscreen or window; resolution; internal resolution; anisotropic filtering; FPS cap; vsync; presentation shader (Original, grayscale, light CRT); brightness, contrast and saturation; on-screen FPS, with a choice of corner |
| Áudio | Master volume; left/right balance; mute (F9); mute when the window loses focus |
| Controles > Controle (controller) | Remap each game button (press A on the button, then press the new physical button); Xbox and PS2 layouts; swap A and B; invert the right stick's horizontal camera (the game's own option only inverts the vertical axis); disable the L3 + R3 shortcut |
| Controles > Teclado e mouse (keyboard and mouse) | Remap keys; enable the keyboard; mouse as camera; invert the mouse on each axis; sensitivity; reset to default |

### Mods, Suporte (support) and Menu

| Tab | What it has |
|---|---|
| Mods | Lists the mods in `mods/`, with a checkbox for each; reload the list; export textures to build an HD pack |
| Suporte | **Relatar bug** (report a bug): game frame capture, title and text, saved in `reports/`. **Capturas salvas** (saved captures): the 12 newest. **Logs e travamentos** (logs and crashes): last log lines and crash dumps. **Diagnóstico** (diagnostics): performance logging, enemy HP probe, current area and room. Nothing is sent to the internet. |
| Menu | **Aparência** (appearance): opacity, colors, pause while the menu is open. **Perfis** (profiles): save and apply sets of options, Dev profile and vanilla defaults. |

## Bundled mods

| Mod | What it does | How to enable |
|---|---|---|
| PTBR-PS2 | Brazilian Portuguese translation of 2479 texts, images and dub, taken from the fans' PS2 ISO | Options > Language > Português in the game (also in Jogabilidade > Jogo e minimapa) |
| HD4K | 2739 4K textures from the PS2 pack (PlatinumMimic), matched to the Xbox ones | Mods tab |
| PS2-Botoes | ✕ ○ □ △ L1 L2 R1 R2 icons instead of the Xbox buttons | Mods tab; for the triggers, also use Controles > Layout PS2 |
| Editor | Per-type enemy HP and size | Jogabilidade > Inimigos: por tipo > "Aplicar edições" |

Mods never change the original game files: they replace them in memory only. To make your own, see [MODDING.md](../MODDING.md). Credits for third-party material are in [CREDITS.md](CREDITS.md).

## What changes from the original Xbox game

- **Video:** runs on Windows with Direct3D 12, at any resolution, with higher internal resolution, anisotropic filtering and on-screen FPS. The character shadow was fixed (4X multisample).
- **Language:** Brazilian Portuguese translation and dub, with language switching during play.
- **Textures:** 4K pack and PS2 button icons.
- **Controls:** keyboard and mouse; Xbox, PlayStation and generic controllers (SDL3); remapping; horizontal camera inversion.
- **Tools:** developer menu with search, alternative cameras and an on-screen minimap.
- **Gameplay:** damage-based difficulty; enemy HP, size, random rolls and elites; drop rate, crystals and random items; HP, MP, gold and EXP cheats; Moai bonus without the PS2 save.
- **Support:** bug report with capture and log, performance logging and crash dumps.

## Known limits

- **Enemy and item size:** fixed on 2026-10-26 to keep the size when the game rescales the enemy. It still needs confirmation in game.
- **Shadow:** still looks wrong in some places. Send an F3 capture when it happens.
- **Occasional FPS drops:** under investigation; turn on performance logging and send a report.
- **Being researched, not available yet:** swapping enemy types, more enemies per room and moving enemies.
- **Not tested in this version:** the @CRAZY and @TREVOR name codes, savestates and switching characters mid-campaign.
