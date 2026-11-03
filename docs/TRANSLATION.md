# Brazilian Portuguese translation

English · [Português (Brasil)](#pt-br)

Choose it in the game itself: **Options → Language → Português (Brasil)** and confirm with **Sim** (yes).
The choices are English, Japanese and Português (Brasil). The same screen is in the pause menu
(Options → Language) during play.

Portuguese turns everything on at once:

- texts: catalog from the translated PS2 ISO, embedded in the EXE;
- images and dub: the `mods/PTBR-PS2` mod, copied from the PS2 patch by `tools/ps2_*.py`.

The choice is saved. Texts change immediately. Images and voices already loaded change
after restarting the game. Without the `PTBR-PS2` folder, Portuguese translates only the texts.

The pack only follows the language: on the Mods screen it shows as locked.

## Build the mod from the ISOs

```powershell
python -m pip install -r tools/requirements-audio.txt
python tools/ps2_movie_dub.py --ptbr-iso <PT-BR ISO> --orig-iso <original ISO>
python tools/ps2_voice_dub.py --ptbr-iso <PT-BR ISO> --orig-iso <original ISO>
python tools/ps2_texture_import.py --ptbr-iso <PT-BR ISO> --orig-iso <original ISO>
```

## Edit a texture of the mod

```powershell
python tools/dar_texture.py export --id 14452 --dar out/build/release/mods/PTBR-PS2/files/data/titleue.dar --png title.png
python tools/dar_texture.py import --id 14452 --png title.png
```

Keep the image size. The import replaces the ID in every DAR in
`mods/PTBR-PS2/files/data` and keeps the previous ones in `.local/ptbr-ps2/dar-backup`.

For high resolution (any size up to 16384), without touching the DAR:

```powershell
python tools/dar_texture.py hd --id 14452 --dar out/build/release/mods/PTBR-PS2/files/data/titleue.dar --png remake.png
```

It writes `mods/PTBR-PS2/textures/<hash>.bmp`. The hash comes from the texture in the DAR;
if you import the ID again later, generate the HD file again. It only applies with Portuguese on.

Reports go to `.local/ptbr-ps2/`. What the patch does not cover is listed in
`.local/ptbr-ps2/FALTANDO.md` (local). Do not publish files derived from the game
together with the project's code.

The runtime replaces texts by ID, without changing the extracted files or the saves.
The local catalog lives in `assets/ptbr_strings.private.h`; the public repository includes
only the adapter, with an empty catalog when that private file is missing.
Do not distribute the catalog derived from the game's texts with the public code.

## Check the local catalog

After preparing your local data:

```powershell
python tools/ptbr_catalog.py --require-complete
python -m unittest tools.test_ptbr
```

The audit reads the `assets/data/strdataue.dar` table and checks every non-empty ID,
button codes, colors, parameters, duplicates and the space available in the runtime.
The report goes to `.local/ptbr-work/coverage.json`, without copying the sentences to the
terminal output. Proper names, symbols and terms defined by the glossary may stay unchanged.

## Coverage limits

Covering the whole table does not prove that every visible text is translated.
The report flags longer texts; it does not measure the real glyph width.
Context, alignment, subtitles and panel space need visual testing in game.

Before changing an entry, keep a private copy of the previous catalog.
Never replace IDs, remove button codes or translate parameters such as `[port]` and `[size]`.
The extracted data and the saves must stay intact.

---

<a id="pt-br"></a>

# Tradução PT-BR

[English](#brazilian-portuguese-translation) · Português (Brasil)

Escolha no próprio jogo: **Opções → Idioma → Português (Brasil)** e confirme com **Sim**.
As opções são English, Japanese e Português (Brasil). A mesma tela fica na pausa
(Opções → Idioma) durante a partida.

Português liga tudo de uma vez:

- textos: catálogo da ISO PS2 traduzida, embutido no EXE;
- imagens e dublagem: mod `mods/PTBR-PS2`, copiado do patch PS2 por `tools/ps2_*.py`.

A escolha fica salva. Textos mudam na hora. Imagens e vozes já carregadas mudam
depois de reiniciar o jogo. Sem a pasta `PTBR-PS2`, Português traduz só os textos.

O pacote segue só o idioma: na tela Mods ele aparece travado.

## Gerar o mod a partir das ISOs

```powershell
python -m pip install -r tools/requirements-audio.txt
python tools/ps2_movie_dub.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original>
python tools/ps2_voice_dub.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original>
python tools/ps2_texture_import.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original>
```

## Editar uma textura do mod

```powershell
python tools/dar_texture.py export --id 14452 --dar out/build/release/mods/PTBR-PS2/files/data/titleue.dar --png titulo.png
python tools/dar_texture.py import --id 14452 --png titulo.png
```

Mantenha o tamanho da imagem. O import troca o ID em todos os DARs de
`mods/PTBR-PS2/files/data` e guarda os anteriores em `.local/ptbr-ps2/dar-backup`.

Para alta resolução (qualquer tamanho até 16384), sem mexer no DAR:

```powershell
python tools/dar_texture.py hd --id 14452 --dar out/build/release/mods/PTBR-PS2/files/data/titleue.dar --png remake.png
```

Grava `mods/PTBR-PS2/textures/<hash>.bmp`. O hash vem da textura que está no DAR;
se reimportar o ID depois, gere o HD de novo. Vale só com Português ativo.

Relatórios ficam em `.local/ptbr-ps2/`. O que o patch não cobre está em
`.local/ptbr-ps2/FALTANDO.md` (local). Não publique arquivos derivados do jogo
junto ao código do projeto.

O runtime substitui os textos por ID, sem modificar os arquivos extraídos ou os saves.
O catálogo local fica em `assets/ptbr_strings.private.h`; o repositório público inclui
somente o adaptador, com catálogo vazio quando esse arquivo privado está ausente.
Não distribua o catálogo derivado dos textos do jogo junto ao código público.

## Verificar o catálogo local

Após preparar seus dados locais:

```powershell
python tools/ptbr_catalog.py --require-complete
python -m unittest tools.test_ptbr
```

A auditoria lê a tabela `assets/data/strdataue.dar`, confere todos os IDs não vazios,
códigos de botões, cores, parâmetros, duplicatas e espaço disponível no runtime.
O relatório fica em `.local/ptbr-work/coverage.json`, sem copiar as frases para a saída
do terminal. Nomes próprios, símbolos e termos definidos pelo glossário podem permanecer iguais.

## Limites da cobertura

Cobrir a tabela inteira não comprova traduzir todo texto visível.
O relatório sinaliza expansões de comprimento; ele não mede a largura real dos glifos.
Ajuste de contexto, alinhamento, legendas e espaço nos painéis exige teste visual em jogo.

Antes de alterar uma entrada, preserve uma cópia privada do catálogo anterior.
Nunca substitua IDs, remova códigos de botão ou traduza parâmetros como `[port]` e `[size]`.
Os dados extraídos e os saves devem continuar intactos.
