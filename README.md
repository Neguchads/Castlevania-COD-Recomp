# Castlevania: Curse of Darkness Recomp

English · [Português (Brasil)](#pt-br)

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

## Download and play (no build)

1. Download `Castlevania-COD-Recomp-<version>-win64.zip` from [Releases](../../releases) and extract it to a folder with about 4 GB free.
2. Put your own copy of *Curse of Darkness* for Xbox (USA) in `assets\`: the `.iso`, or `default.xbe` with the game folders. The zip does not include the game.
3. Open `CastlevaniaCOD.exe`. On the first start it builds the game code from your copy (with a progress window; it can take a long time, do not close it); after that it opens directly.
4. Optional: the HD4K, PTBR-PS2 and PS2-Botoes packs are linked in the release description. Extract each zip into the game folder (it creates `mods\<pack>`) and turn it on in the Mods tab of the tools menu (F1). PT-BR shows up under Options > Language.

Check a download with `Get-FileHash <file>.zip` in PowerShell against the release's `SHA256SUMS.txt`. The zip ships the Visual C++ runtime beside the exe; if Windows still reports a missing DLL, install the Visual C++ Redistributable 2015–2022 x64.

## Build from source

Build requirements: Windows 10/11, Visual Studio 2022 with "Desktop development with C++" and "C++ Clang tools", Git, Python 3.12, and your own copy of *Curse of Darkness* for Xbox (USA).

1. Put `default.xbe` and the game folders in `assets/`.
2. Keep the `XboxXBERecomp` checkout next to this project. The SDK fallback is pinned to commit `e0de3e44d10443c1c9aad512f36baf133cdd4a60`.
3. Run `build.bat`. The first build recompiles the XBE (30 to 90 minutes).
4. Open `out\build\release\CastlevaniaCOD.exe`. **F1** or **L3 + R3** opens the tools menu.
5. Optional: `.\tools\release.ps1 -Destination "<games folder>\Castlevania Curse of Darkness Recomp"` builds, runs the tests and only then updates the play folder and a desktop shortcut; the destination is remembered in `.local\release-destination.txt` and extra tool folders (such as LLVM) go one per line in `.local\tool-path.txt`. Underneath, `.\tools\make-portable.ps1` builds a ready-to-play folder, "Castlevania Curse of Darkness Recomp", on the desktop, with the exe, dll, `assets`, mods, saves and settings. It works from any disk. When run again, the script updates the game and the mods but does not overwrite the folder's saves or settings. `-Destination` picks another place and `-DryRun` only shows the size. The folder needs only the Visual C++ Redistributable 2015–2022 x64 and a DirectX 12 GPU. Copy the whole folder and keep the file dates: without the `compiler` folder, the game only uses the dll if it is newer than the exe.
6. Optional: your own `assets/icon.ico` becomes the exe and window icon on the next build (resource `ICON 1`, compiled with `llvm-rc`). The file stays out of git.

Before a commit or PR, run `tools\verify.cmd`: build, C++ and Python tests (`tools\test-local.cmd`) and a gitleaks secret scan.

The translation, dub and 4K pack are generated from the user's own copies; see [TRANSLATION.md](docs/TRANSLATION.md) and [MODDING.md](MODDING.md).

## Folder layout

Keep three folders apart, so the build tools never end up in the folder you play from:

| Folder | Purpose | Contents |
|---|---|---|
| Source (this repository) | Code and docs that go to GitHub | `src/`, `tools/`, `docs/`, tests |
| Build workspace (a checkout with your `assets/`) | Build and test | `assets/`, `.local/` tools, `out/build/release` with the `compiler` folder, test captures and reports |
| Play folder (release) | Only play | exe, dll, `assets`, mods, saves, settings and guides; no compiler |

`tools\release.ps1` is the only way into the play folder: it builds, runs `tools\test-local.cmd`, and only then calls `make-portable.ps1` and creates a desktop shortcut. If the build or a test fails, the play folder is not touched. Put the play folder on a data disk (for example a `Jogos` or `Games` folder) instead of the desktop: it is 6.6 GB.

## Demo

Screenshots and video are still to come. In game: tools menu (F1), free and orbital cameras, English/Portuguese language in the pause menu, HD4K pack and PS2-Botoes in the Mods tab.

## Not included

This repository does not distribute ISO/XBE files, extracted files, the game's generated C++ code, the recompiled DLL, saves, captures, the PT-BR text catalog, or the translation, dub or 4K pack files. All of that is generated locally from the user's own copies. The release zip follows the same rule: it has the public exe (built with `-DCOD_PUBLIC_BUILD=ON`, no PT-BR text inside) and the `compiler` folder, whose `config/game.toml` lists only addresses of the XBE's functions, which the first start needs to build the game code. The optional packs are hosted outside GitHub and credited in [CREDITS.md](docs/CREDITS.md).

## Credits and status

The Brazilian Portuguese translation and dub are by HNNEWGAMES, STR Brasil, MATTH dubs and Os Aristocratas do Giz Vermelho. The 4K texture pack is by PlatinumMimic. Authors, roles and links are in [CREDITS.md](docs/CREDITS.md).

The port is experimental and the full campaign has not been validated. What is validated in game, what only passed tests and what is experimental by design is listed in [Experimental features and status](docs/GUIDE.md#experimental-features-and-status). Four maps without teleporters still have no name.

No license has been chosen yet for this project's changes: the code is visible, but that does not grant permission to reuse it. Runtime parts derived from XboxXBERecomp follow the MIT notice in [LICENSES/XboxXBERecomp-MIT.txt](LICENSES/XboxXBERecomp-MIT.txt); the SDK and the other dependencies keep their own licenses.

---

<a id="pt-br"></a>

# Castlevania: Curse of Darkness Recomp

[English](#castlevania-curse-of-darkness-recomp) · Português (Brasil)

Port experimental de *Castlevania: Curse of Darkness* (Xbox, 2005) para Windows, por recompilação estática. O código x86 do XBE original vira C++ nativo, e um runtime próprio substitui os serviços do Xbox: Direct3D 8 sobre Direct3D 12, áudio em XAudio2, controle por SDL3 e arquivos no disco local.

## Problema que resolve

O jogo só roda em Xbox original ou em emulador. Emulação de Xbox é pesada e ainda tem falhas gráficas neste título. A versão PS2 tem tradução e dublagem em português feitas por fãs e um pacote de texturas 4K, mas a versão Xbox, com sombras e texturas melhores, não tinha nada disso. Este projeto:

- roda o jogo nativo no Windows, em qualquer resolução, sem emulador;
- traz para o Xbox a tradução PT-BR, a dublagem e o pacote 4K do PS2, convertidos a partir das cópias do próprio usuário;
- adiciona um menu de desenvolvimento com opções que o original não tem (câmera livre, dificuldade, cheats, mapeamento de controle, relato de bugs).

## Tecnologias e por quê

| Peça | Por quê |
|---|---|
| [XboxXBERecomp](https://github.com/TGP482/XboxXBERecomp) (recompilação estática x86) | Gera C++ do XBE uma vez; o jogo roda em velocidade nativa, sem interpretar instruções. |
| C++20 + Clang + CMake/Ninja | É o toolchain exigido pelo SDK de recompilação; o mesmo compilador serve para o código gerado e para o runtime. |
| Direct3D 12 | Traduz o pipeline fixo e os shaders do D3D8 do Xbox para uma API atual. No Linux, o Proton roda o D3D12 sobre Vulkan. |
| Dear ImGui | Menu dev dentro do jogo, navegável por controle e teclado, sem dependência de janela externa. |
| SDL3 | Controles de Xbox, PlayStation e genéricos com um código só. |
| Python (Pillow, NumPy) | Ferramentas offline: conversão de texturas e áudio do PS2, pareamento do pacote 4K com as texturas do Xbox. |
| texconv (DirectXTex) | Comprime as texturas HD em BC7 com mipmaps, que a placa de vídeo lê direto. |

## Destaques técnicos

- **Hooks em funções recompiladas:** cada função do jogo pode ser substituída mantendo a original. Godmode, dificuldade e economia (ouro/EXP) passam pelo setter de atributos do próprio jogo, e o bônus do save de *Lament of Innocence* (estátua Moai) é liberado respondendo uma flag no leitor de flags.
- **Sombra corrigida:** o volume de sombra usa o modo multisample 4X do XDK, em que o hardware expande uma grade lógica 2×2. O port ignorava isso e desenhava a sombra num quarto da tela. Veja `src/common/multisample_rules.h`.
- **Texturas HD sem queda de FPS:** o hash da textura é guardado por quadro (antes eram 4,6 ms por quadro medidos) e o pacote é indexado uma vez, sem consultar o disco na thread de render.
- **Pacote 4K do PS2 no Xbox:** `tools/hd_pack_match.py` compara cada PNG do pacote com as texturas do Xbox por assinatura de cor e aceita só pares com folga para o segundo colocado. Texturas quase iguais recebem só o detalhe fino do 4K. São 2739 texturas pareadas.
- **Testes:** regras puras (dano, economia, controle, navegação do menu, multisample, índice de texturas, editor de inimigos, drops, busca do menu) têm testes C++, as ferramentas têm testes Python, e as funções do jogo são exercitadas numa memória isolada no autoteste do runtime.

## Requisitos de PC

| | Mínimo | Testado em |
|---|---|---|
| Sistema | Windows 10 ou 11, 64 bits | Windows 11 Pro |
| Processador | x86-64 com SSE4.2 e POPCNT (`-march=x86-64-v2`) | Intel Core i5-10400F |
| Placa de vídeo | Direct3D 12, nível 11_0 | NVIDIA GeForce RTX 3060, 12 GB |
| Memória | Não medida | 32 GB |
| Disco | 6,6 GB com HD4K e PT-BR; cerca de 3,3 GB sem o HD4K | SSD |
| Bibliotecas | Visual C++ Redistributable 2015–2022 x64 | — |

Na máquina de teste o jogo roda em 4K, com resolução interna de 200% e 60 FPS. O mínimo vem do que o código exige; não houve teste em PC mais fraco.

## Guia do jogador

[docs/GUIDE.md](docs/GUIDE.md#pt-br) explica atalhos, cada aba e opção do menu, os mods incluídos, os cheats, a pasta pronta para jogar, os saves e o que muda em relação ao Xbox original.

## Baixar e jogar (sem compilar)

1. Baixe `Castlevania-COD-Recomp-<versão>-win64.zip` em [Releases](../../releases) e extraia numa pasta com uns 4 GB livres.
2. Coloque a sua cópia de *Curse of Darkness* para Xbox (USA) em `assets\`: a `.iso`, ou `default.xbe` com as pastas do jogo. O zip não traz o jogo.
3. Abra `CastlevaniaCOD.exe`. Na primeira abertura ele gera o código do jogo a partir da sua cópia (com janela de progresso; pode demorar bastante, não feche); depois abre direto.
4. Opcional: os pacotes HD4K, PTBR-PS2 e PS2-Botoes têm links na descrição da release. Extraia cada zip na pasta do jogo (eles criam `mods\<pacote>`) e ative na aba Mods do menu (F1). O PT-BR aparece em Opções > Idioma.

Confira o download com `Get-FileHash <arquivo>.zip` no PowerShell e compare com o `SHA256SUMS.txt` da release. O zip leva o Visual C++ runtime ao lado do exe; se o Windows reclamar de DLL, instale o Visual C++ Redistributable 2015–2022 x64.

## Como compilar

Requisitos para compilar: Windows 10/11, Visual Studio 2022 com "Desenvolvimento para desktop com C++" e "C++ Clang tools", Git, Python 3.12, e sua própria cópia de *Curse of Darkness* para Xbox (USA).

1. Coloque `default.xbe` e as pastas do jogo em `assets/`.
2. Mantenha o checkout do `XboxXBERecomp` ao lado deste projeto. O fallback do SDK fica fixado no commit `e0de3e44d10443c1c9aad512f36baf133cdd4a60`.
3. Rode `build.bat`. O primeiro build recompila o XBE (30 a 90 minutos).
4. Abra `out\build\release\CastlevaniaCOD.exe`. **F1** ou **L3 + R3** abre o menu dev.
5. Opcional: `.\tools\release.ps1 -Destination "<pasta de jogos>\Castlevania Curse of Darkness Recomp"` faz o build, roda os testes e só então atualiza a pasta de jogar e um atalho na área de trabalho; o destino fica guardado em `.local\release-destination.txt` e pastas extras de ferramentas (como o LLVM) vão uma por linha em `.local\tool-path.txt`. Por baixo, `.\tools\make-portable.ps1` monta uma pasta pronta para jogar, "Castlevania Curse of Darkness Recomp", na área de trabalho, com exe, dll, `assets`, mods, saves e opções. Ela funciona em qualquer disco. Ao rodar de novo, o script atualiza o jogo e os mods, mas não sobrescreve os saves nem as opções da pasta pronta. `-Destination` escolhe outro lugar e `-DryRun` só mostra o tamanho. A pasta precisa só do Visual C++ Redistributable 2015–2022 x64 e de uma placa com DirectX 12. Copie a pasta inteira e mantenha as datas dos arquivos: sem a pasta `compiler`, o jogo só usa a dll se ela for mais nova que o exe.
6. Opcional: um `assets/icon.ico` seu vira o ícone do exe e da janela no próximo build (recurso `ICON 1`, compilado com `llvm-rc`). O arquivo fica fora do git.

Antes de um commit ou PR, rode `tools\verify.cmd`: build, testes C++ e Python (`tools\test-local.cmd`) e varredura de segredos com gitleaks.

Tradução, dublagem e o pacote 4K são gerados a partir das cópias do usuário; veja [TRANSLATION.md](docs/TRANSLATION.md#pt-br) e [MODDING.md](MODDING.md#pt-br).

## Organização das pastas

Mantenha três pastas separadas, para as ferramentas de build nunca irem parar na pasta de jogar:

| Pasta | Para quê | O que tem |
|---|---|---|
| Código (este repositório) | Código e documentação que vão para o GitHub | `src/`, `tools/`, `docs/`, testes |
| Pasta de build (um checkout com os seus `assets/`) | Montar e testar | `assets/`, ferramentas em `.local/`, `out/build/release` com a pasta `compiler`, capturas e relatos de teste |
| Pasta de jogar (release) | Só jogar | exe, dll, `assets`, mods, saves, opções e guias; sem compilador |

`tools\release.ps1` é o único caminho para a pasta de jogar: ele faz o build, roda `tools\test-local.cmd` e só então chama o `make-portable.ps1` e cria um atalho na área de trabalho. Se o build ou um teste falhar, a pasta de jogar não é tocada. Deixe a pasta de jogar num disco de dados (por exemplo uma pasta `Jogos`), não na área de trabalho: ela tem 6,6 GB.

## Demonstração

Capturas e vídeo ainda vão entrar aqui. No jogo: menu dev (F1), câmera livre e orbital, idioma English/Português na pausa, pacote HD4K e PS2-Botoes na aba Mods.

## Não inclui

Este repositório não distribui ISO/XBE, arquivos extraídos, o código C++ gerado do jogo, a DLL recompilada, saves, capturas, o catálogo de textos PT-BR nem os arquivos da tradução, da dublagem ou do pacote 4K. Tudo isso é gerado localmente a partir das cópias do usuário. O zip da release segue a mesma regra: leva o exe público (compilado com `-DCOD_PUBLIC_BUILD=ON`, sem texto PT-BR dentro) e a pasta `compiler`, cujo `config/game.toml` lista só endereços das funções do XBE, que a primeira abertura usa para gerar o código do jogo. Os pacotes opcionais ficam hospedados fora do GitHub, com créditos em [CREDITS.md](docs/CREDITS.md#pt-br).

## Créditos e estado

A tradução e a dublagem PT-BR são de HNNEWGAMES, STR Brasil, MATTH dubs e Os Aristocratas do Giz Vermelho. O pacote de texturas 4K é de PlatinumMimic. Autores, funções e links estão em [CREDITS.md](docs/CREDITS.md#pt-br).

O port é experimental e a campanha inteira não foi validada. O que foi validado no jogo, o que só passou nos testes e o que é experimental por natureza está em [Recursos experimentais e estado](docs/GUIDE.md#recursos-experimentais-e-estado). Quatro mapas sem teleporte ainda não têm nome.

Ainda não foi escolhida uma licença para as alterações deste projeto: o código fica visível, mas isso não concede permissão de reutilização. Partes do runtime derivadas do XboxXBERecomp seguem o aviso MIT em [LICENSES/XboxXBERecomp-MIT.txt](LICENSES/XboxXBERecomp-MIT.txt); o SDK e as demais dependências mantêm suas licenças próprias.
