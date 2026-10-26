# Castlevania: Curse of Darkness Recomp

English · [Português (Brasil)](README.pt-BR.md)

Experimental Windows port of *Castlevania: Curse of Darkness* (Xbox, 2005), made by static recompilation. The x86 code of the original XBE becomes native C++, and a custom runtime replaces the Xbox services: Direct3D 8 on top of Direct3D 12, XAudio2 audio, SDL3 controllers and files on the local disk.

## The problem it solves

The game only runs on an original Xbox or in an emulator. Xbox emulation is heavy and still has graphics bugs in this title. The PS2 version has a fan-made Brazilian Portuguese translation and dub, plus a 4K texture pack, but the Xbox version, which has better shadows and textures, had none of that. This project:

- runs the game natively on Windows, at any resolution, without an emulator;
- brings the PS2 Brazilian Portuguese translation, dub and 4K pack to the Xbox version, converted from the user's own copies;
- adds a developer menu with options the original does not have (free camera, difficulty, cheats, controller remapping, bug reports).

## Technologies and why

| Piece | Why |
|---|---|
| [XboxXBERecomp](https://github.com/TGP482/XboxXBERecomp) (x86 static recompilation) | Generates C++ from the XBE once; the game runs at native speed, without interpreting instructions. |
| C++20 + Clang + CMake/Ninja | The toolchain required by the recompilation SDK; the same compiler builds the generated code and the runtime. |
| Direct3D 12 | Translates the Xbox D3D8 fixed pipeline and shaders to a current API. On Linux, Proton runs D3D12 on Vulkan. |
| Dear ImGui | In-game developer menu, usable with a controller or keyboard, with no external window. |
| SDL3 | Xbox, PlayStation and generic controllers with one code path. |
| Python (Pillow, NumPy) | Offline tools: conversion of PS2 textures and audio, matching the 4K pack to the Xbox textures. |
| texconv (DirectXTex) | Compresses the HD textures to BC7 with mipmaps, which the GPU reads directly. |

## Technical highlights

- **Hooks on recompiled functions:** any game function can be replaced while keeping the original callable. Godmode, difficulty and economy (gold/EXP) go through the game's own attribute setter, and the *Lament of Innocence* save bonus (Moai statue) is unlocked by answering a flag in the flag reader.
- **Shadow fix:** the shadow volume uses the XDK 4X multisample mode, where the hardware expands a 2×2 logical grid. The port ignored this and drew the shadow in a quarter of the screen. See `src/common/multisample_rules.h`.
- **HD textures without FPS drops:** the texture hash is cached per frame (it cost 4.6 ms per frame, measured) and the pack is indexed once, without disk lookups on the render thread.
- **PS2 4K pack on Xbox:** `tools/hd_pack_match.py` compares each PNG in the pack with the Xbox textures by color signature and only accepts pairs with a clear margin over the runner-up. Near-identical textures receive only the fine 4K detail. 2739 textures are matched.
- **Tests:** pure rules (damage, economy, controller, menu navigation, multisample, texture index, enemy editor, drops, menu search) have C++ tests, the tools have Python tests, and game functions are exercised in isolated memory by the runtime self-test.

## PC requirements

| | Minimum | Tested on |
|---|---|---|
| OS | Windows 10 or 11, 64-bit | Windows 11 Pro |
| CPU | x86-64 with SSE4.2 and POPCNT (`-march=x86-64-v2`) | Intel Core i5-10400F |
| GPU | Direct3D 12, feature level 11_0 | NVIDIA GeForce RTX 3060, 12 GB |
| RAM | Not measured | 32 GB |
| Disk | 6.6 GB with HD4K and PT-BR; about 3.3 GB without HD4K | SSD |
| Libraries | Visual C++ Redistributable 2015–2022 x64 | — |

On the test machine the game runs at 4K with 200% internal resolution and 60 FPS. The minimum comes from what the code requires; it was not tested on a weaker PC.

## Player guide

[docs/GUIDE.md](docs/GUIDE.md) covers hotkeys, every tab and option of the tools menu, the bundled mods, the cheats, the ready-to-play folder, saves and what changes compared to the original Xbox game.

## Install and run

Build requirements: Windows 10/11, Visual Studio 2022 with "Desktop development with C++" and "C++ Clang tools", Git, Python 3.12, and your own copy of *Curse of Darkness* for Xbox (USA).

1. Put `default.xbe` and the game folders in `assets/`.
2. Keep the `XboxXBERecomp` checkout next to this project. The SDK fallback is pinned to commit `e0de3e44d10443c1c9aad512f36baf133cdd4a60`.
3. Run `build.bat`. The first build recompiles the XBE (30 to 90 minutes).
4. Open `out\build\release\CastlevaniaCOD.exe`. **F1** or **L3 + R3** opens the tools menu.
5. Optional: `.\tools\make-portable.ps1` builds a ready-to-play folder, "Castlevania Curse of Darkness Recomp", on the desktop, with the exe, dll, `assets`, mods, saves and settings. It works from any disk. When run again, the script updates the game and the mods but does not overwrite the folder's saves or settings. `-Destination` picks another place and `-DryRun` only shows the size. The folder needs only the Visual C++ Redistributable 2015–2022 x64 and a DirectX 12 GPU. Copy the whole folder and keep the file dates: without the `compiler` folder, the game only uses the dll if it is newer than the exe.
6. Optional: your own `assets/icon.ico` becomes the exe and window icon on the next build (resource `ICON 1`, compiled with `llvm-rc`). The file stays out of git.

Before a commit or PR, run `tools\verify.cmd`: build, C++ and Python tests (`tools\test-local.cmd`) and a gitleaks secret scan.

The translation, dub and 4K pack are generated from the user's own copies; see [TRANSLATION.md](docs/TRANSLATION.md) and [MODDING.md](MODDING.md).

## Demo

Screenshots and video are still to come. In game: tools menu (F1), free and orbital cameras, English/Portuguese language in the pause menu, HD4K pack and PS2-Botoes in the Mods tab.

## Not included

This repository does not distribute ISO/XBE files, extracted files, the game's generated C++ code, the recompiled DLL, saves, captures, the PT-BR text catalog, or the translation, dub or 4K pack files. All of that is generated locally from the user's own copies.

## Credits and status

The Brazilian Portuguese translation and dub are by HNNEWGAMES, STR Brasil, MATTH dubs and Os Aristocratas do Giz Vermelho. The 4K texture pack is by PlatinumMimic. Authors, roles and links are in [CREDITS.md](docs/CREDITS.md).

The port is experimental and the full campaign has not been validated. Enemy HP works (validated in game); enemy and item size was fixed but still needs confirmation in game. Four maps without teleporters still have no name.

No license has been chosen yet for this project's changes: the code is visible, but that does not grant permission to reuse it. Runtime parts derived from XboxXBERecomp follow the MIT notice in [LICENSES/XboxXBERecomp-MIT.txt](LICENSES/XboxXBERecomp-MIT.txt); the SDK and the other dependencies keep their own licenses.
