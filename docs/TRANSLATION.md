# Brazilian Portuguese translation

English · [Português (Brasil)](TRANSLATION.pt-BR.md)

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
