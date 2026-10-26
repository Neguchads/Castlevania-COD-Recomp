# Castlevania: Curse of Darkness Recomp

[English](README.md) · Português (Brasil)

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

[docs/GUIDE.pt-BR.md](docs/GUIDE.pt-BR.md) explica atalhos, cada aba e opção do menu, os mods incluídos, os cheats, a pasta pronta para jogar, os saves e o que muda em relação ao Xbox original.

## Como instalar e rodar

Requisitos para compilar: Windows 10/11, Visual Studio 2022 com "Desenvolvimento para desktop com C++" e "C++ Clang tools", Git, Python 3.12, e sua própria cópia de *Curse of Darkness* para Xbox (USA).

1. Coloque `default.xbe` e as pastas do jogo em `assets/`.
2. Mantenha o checkout do `XboxXBERecomp` ao lado deste projeto. O fallback do SDK fica fixado no commit `e0de3e44d10443c1c9aad512f36baf133cdd4a60`.
3. Rode `build.bat`. O primeiro build recompila o XBE (30 a 90 minutos).
4. Abra `out\build\release\CastlevaniaCOD.exe`. **F1** ou **L3 + R3** abre o menu dev.
5. Opcional: `.\tools\release.ps1 -Destination "<pasta de jogos>\Castlevania Curse of Darkness Recomp"` faz o build, roda os testes e só então atualiza a pasta de jogar e um atalho na área de trabalho; o destino fica guardado em `.local\release-destination.txt` e pastas extras de ferramentas (como o LLVM) vão uma por linha em `.local\tool-path.txt`. Por baixo, `.\tools\make-portable.ps1` monta uma pasta pronta para jogar, "Castlevania Curse of Darkness Recomp", na área de trabalho, com exe, dll, `assets`, mods, saves e opções. Ela funciona em qualquer disco. Ao rodar de novo, o script atualiza o jogo e os mods, mas não sobrescreve os saves nem as opções da pasta pronta. `-Destination` escolhe outro lugar e `-DryRun` só mostra o tamanho. A pasta precisa só do Visual C++ Redistributable 2015–2022 x64 e de uma placa com DirectX 12. Copie a pasta inteira e mantenha as datas dos arquivos: sem a pasta `compiler`, o jogo só usa a dll se ela for mais nova que o exe.
6. Opcional: um `assets/icon.ico` seu vira o ícone do exe e da janela no próximo build (recurso `ICON 1`, compilado com `llvm-rc`). O arquivo fica fora do git.

Antes de um commit ou PR, rode `tools\verify.cmd`: build, testes C++ e Python (`tools\test-local.cmd`) e varredura de segredos com gitleaks.

Tradução, dublagem e o pacote 4K são gerados a partir das cópias do usuário; veja [TRANSLATION.pt-BR.md](docs/TRANSLATION.pt-BR.md) e [MODDING.pt-BR.md](MODDING.pt-BR.md).

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

Este repositório não distribui ISO/XBE, arquivos extraídos, o código C++ gerado do jogo, a DLL recompilada, saves, capturas, o catálogo de textos PT-BR nem os arquivos da tradução, da dublagem ou do pacote 4K. Tudo isso é gerado localmente a partir das cópias do usuário.

## Créditos e estado

A tradução e a dublagem PT-BR são de HNNEWGAMES, STR Brasil, MATTH dubs e Os Aristocratas do Giz Vermelho. O pacote de texturas 4K é de PlatinumMimic. Autores, funções e links estão em [CREDITS.pt-BR.md](docs/CREDITS.pt-BR.md).

O port é experimental e a campanha inteira não foi validada. O que foi validado no jogo, o que só passou nos testes e o que é experimental por natureza está em [Recursos experimentais e estado](docs/GUIDE.pt-BR.md#recursos-experimentais-e-estado). Quatro mapas sem teleporte ainda não têm nome.

Ainda não foi escolhida uma licença para as alterações deste projeto: o código fica visível, mas isso não concede permissão de reutilização. Partes do runtime derivadas do XboxXBERecomp seguem o aviso MIT em [LICENSES/XboxXBERecomp-MIT.txt](LICENSES/XboxXBERecomp-MIT.txt); o SDK e as demais dependências mantêm suas licenças próprias.
