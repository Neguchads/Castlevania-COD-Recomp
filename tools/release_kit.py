"""Monta os zips de distribuicao, sem dados do jogo.

  python tools/release_kit.py kit   --build out/build/public --version v0.1.0 --out <pasta> --catalog .local/ptbr_strings.bin
  python tools/release_kit.py packs --mods out/build/release/mods --out <pasta> --catalog .local/ptbr_strings.bin

kit: o zip do GitHub. Exe do build publico (-DCOD_PUBLIC_BUILD=ON), pasta compiler sem work/ (o codigo do jogo
e gerado no PC do jogador a partir da ISO dele), runtime do Visual C++, guias e licencas.
packs: um zip por pacote de mods/ (HD4K, PTBR-PS2, PS2-Botoes), para hospedar fora do GitHub. O PT-BR leva
ptbr_strings.bin, o texto que o exe publico nao tem.
Os dois gravam SHA256SUMS.txt na pasta de saida.
"""
import argparse
import hashlib
import re
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = "CastlevaniaCOD.exe"
TOP = "Castlevania-COD-Recomp"
PACKS = ("HD4K", "PTBR-PS2", "PS2-Botoes")
CATALOG_MAGIC = b"CODPTBR1"
CATALOG_FILE = "ptbr_strings.bin"
# extensoes de dado do jogo ou de codigo gerado: nunca no zip do GitHub
GAME_DATA = {".xbe", ".iso", ".xiso", ".dar", ".xwb", ".xpr", ".dds", ".cv2", ".sfd", ".bmp", ".sav", ".obj"}
VC_RUNTIME = ("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll")
# o que game_code.cpp precisa para gerar a DLL no PC do jogador; so isto sai da pasta compiler
COMPILER_FILES = ("clang.exe", "lld-link.exe", "xbe_recomp.exe", "LLVM-LICENSE.txt", "CastlevaniaCOD.lib")
COMPILER_DIRS = ("config", "include", "xdk")
MIN_LEAK_SAMPLES = 20
PRIVATE_PATH = re.compile(rb"(?i)\b[A-Z]:[\\/](?:Users[\\/]|Recomps(?: e Mods)?[\\/])")
# cada guia tem ingles em cima e portugues embaixo (tools/test_doc_links.py)
DOCS = ("README.md", "MODDING.md", "cheats.example.toml", "docs/GUIDE.md", "docs/CREDITS.md", "docs/TRANSLATION.md")
# textos para quem baixa: ingles em cima, portugues embaixo, sem acento (abrem bem no Bloco de Notas)
KIT_README = """Castlevania: Curse of Darkness Recomp
=====================================

ENGLISH

What it is: an experimental Windows port of Castlevania: Curse of Darkness (Xbox, 2005),
made by static recompilation. The game is NOT included: you need your own Xbox (USA) copy.

How to play:
1. Put your copy in the assets folder: the .iso, or default.xbe with the game folders.
2. Double-click CastlevaniaCOD.exe. On the first start it builds the game code from your
   copy (progress window; it can take a while, do not close it). After that it opens directly.
3. Tools menu: F1 on the keyboard or L3 + R3 on the controller.

Optional packs (HD4K textures, PT-BR translation and dub, PS2 buttons), on Google Drive:
https://drive.google.com/drive/folders/14RMjeEP5_lL7bLUTY70CBQKBv3nOjcJk
Extract each zip into this folder (it creates mods/<pack>) and turn it on in F1 > Mods.
Portuguese text: in game, Options > Language.

Guides: docs\\GUIDE.md (player guide), MODDING.md (mods and cheats), docs\\CREDITS.md (authors
of the translation, dub and textures). They open in Notepad.
If Windows reports a missing DLL, install the Microsoft Visual C++ Redistributable 2015-2022 x64.
Requirements: Windows 10/11 64-bit, CPU with SSE4.2, GPU with Direct3D 12.
Castlevania: Curse of Darkness (c) Konami. This project is not affiliated with Konami.

PORTUGUES (BRASIL)

O que e: port experimental para Windows de Castlevania: Curse of Darkness (Xbox, 2005),
feito por recompilacao estatica. O jogo NAO vem junto: voce precisa da sua copia de Xbox (USA).

Como jogar:
1. Coloque a sua copia na pasta assets: a .iso, ou default.xbe com as pastas do jogo.
2. Clique duas vezes em CastlevaniaCOD.exe. Na primeira abertura ele gera o codigo do jogo a
   partir da sua copia (janela de progresso; pode demorar, nao feche). Depois abre direto.
3. Menu de ferramentas: F1 no teclado ou L3 + R3 no controle.

Pacotes opcionais (texturas HD4K, traducao e dublagem PT-BR, botoes do PS2), no Google Drive:
https://drive.google.com/drive/folders/14RMjeEP5_lL7bLUTY70CBQKBv3nOjcJk
Extraia cada zip nesta pasta (ele cria mods/<pacote>) e ative em F1 > Mods.
Texto em portugues: no jogo, Opcoes > Idioma.

Guias: docs\\GUIDE.md (guia do jogador), MODDING.md (mods e cheats), docs\\CREDITS.md (autores
da traducao, dublagem e texturas). Cada um tem ingles em cima e portugues embaixo e abre no
Bloco de Notas.
Se o Windows reclamar de DLL, instale o Microsoft Visual C++ Redistributable 2015-2022 x64.
Requisitos: Windows 10/11 64 bits, processador com SSE4.2, placa com Direct3D 12.
Castlevania: Curse of Darkness (c) Konami. Este projeto nao tem ligacao com a Konami.
"""
ASSETS_README = """ENGLISH
Put your own copy of Castlevania: Curse of Darkness for Xbox (USA) here: the .iso, or default.xbe
with the game folders. On the first start the exe builds the game code from it (it can take a
while; do not close it).

PORTUGUES (BRASIL)
Coloque aqui a sua copia de Castlevania: Curse of Darkness para Xbox (USA): a .iso, ou default.xbe
com as pastas do jogo. Na primeira abertura o exe gera o codigo do jogo a partir dela (pode
demorar; nao feche a janela).
"""
MODS_README = """ENGLISH
Optional packs (downloaded separately): extract each zip into the game folder; it creates
mods/<pack>. HD4K = 4K textures, PTBR-PS2 = PT-BR translation and dub, PS2-Botoes = PS2 button
icons. Turn them on in F1 > Mods.

PORTUGUES (BRASIL)
Pacotes opcionais (baixados a parte): extraia cada zip na pasta do jogo; eles criam
mods/<pacote>. HD4K = texturas 4K, PTBR-PS2 = traducao e dublagem PT-BR, PS2-Botoes = icones de
botao do PS2. Ative em F1 > Mods.
"""
# o que e e como usar cada pacote: (ingles, portugues, uso em ingles, uso em portugues)
PACK_README = {
    "HD4K": ("4K textures for the Xbox version, from the PCSX2 4K pack by PlatinumMimic.",
             "Texturas 4K para a versao de Xbox, a partir do pacote 4K para PCSX2 de PlatinumMimic.",
             "Turn on HD4K in F1 > Mods.", "Ative HD4K em F1 > Mods."),
    "PTBR-PS2": ("Brazilian Portuguese translation and dub from the PS2 patch by HNNEWGAMES, STR Brasil,\n"
                 "MATTH dubs and Os Aristocratas do Giz Vermelho, plus ptbr_strings.bin (the game texts).",
                 "Traducao e dublagem PT-BR do patch de PS2 de HNNEWGAMES, STR Brasil, MATTH dubs e\n"
                 "Os Aristocratas do Giz Vermelho, mais ptbr_strings.bin (os textos do jogo).",
                 "In game: Options > Language > Portugues. Restart the game to load every image and voice.",
                 "No jogo: Opcoes > Idioma > Portugues. Reinicie o jogo para carregar todas as imagens e vozes."),
    "PS2-Botoes": ("PS2 button icons instead of the Xbox ones.", "Icones de botao do PS2 no lugar dos de Xbox.",
                   "Turn on PS2-Botoes in F1 > Mods.", "Ative PS2-Botoes em F1 > Mods."),
}
DRIVE_FOLDER = "https://drive.google.com/drive/folders/14RMjeEP5_lL7bLUTY70CBQKBv3nOjcJk"
RELEASE_FOLDER_README = """Castlevania: Curse of Darkness Recomp - release files
====================================================

ENGLISH

Castlevania-COD-Recomp-<version>-win64.zip  The game port (GitHub release). No game files inside.
Castlevania-COD-Recomp-HD4K.zip              Optional 4K textures (host outside GitHub).
Castlevania-COD-Recomp-PTBR-PS2.zip          Optional PT-BR translation and dub (host outside GitHub).
Castlevania-COD-Recomp-PS2-Botoes.zip        Optional PS2 button icons (host outside GitHub).
SHA256SUMS.txt                               Checksums: Get-FileHash <file>.zip in PowerShell.

Players extract the port zip, put their own Xbox (USA) copy in assets, and extract the
optional packs into the same folder. Made by tools/release_kit.py.

PORTUGUES (BRASIL)

Castlevania-COD-Recomp-<versao>-win64.zip   O port (release do GitHub). Sem arquivos do jogo.
Castlevania-COD-Recomp-HD4K.zip              Texturas 4K opcionais (hospedar fora do GitHub).
Castlevania-COD-Recomp-PTBR-PS2.zip          Traducao e dublagem PT-BR opcionais (fora do GitHub).
Castlevania-COD-Recomp-PS2-Botoes.zip        Icones de botao do PS2 opcionais (fora do GitHub).
SHA256SUMS.txt                               Conferencia: Get-FileHash <arquivo>.zip no PowerShell.

Quem joga extrai o zip do port, coloca a propria copia de Xbox (USA) em assets e extrai os
pacotes opcionais na mesma pasta. Gerado por tools/release_kit.py.
"""


def pack_readme(pack: str) -> str:
    what_en, what_pt, use_en, use_pt = PACK_README[pack]
    return (f"Castlevania: Curse of Darkness Recomp - {pack}\n\nENGLISH\n{what_en}\n"
            f"Install: extract this zip into the game folder (next to CastlevaniaCOD.exe).\n{use_en}\n"
            "Credits and the authors' pages: docs\\CREDITS.md in the game folder.\n\n"
            f"PORTUGUES (BRASIL)\n{what_pt}\n"
            f"Instalar: extraia este zip na pasta do jogo (ao lado do CastlevaniaCOD.exe).\n{use_pt}\n"
            "Creditos e paginas dos autores: docs\\CREDITS.md na pasta do jogo.\n")


def crlf(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\n", "\r\n")


def write_folder_readme(out: Path) -> None:
    (out / "README.txt").write_bytes(crlf(RELEASE_FOLDER_README).encode("utf-8"))


def parse_catalog(data: bytes) -> dict:
    """Mesmo formato de src/common/ptbr_catalog.h; ValueError se nao for um catalogo valido."""
    if len(data) < 12 or data[:8] != CATALOG_MAGIC:
        raise ValueError("not a CODPTBR1 catalog")
    (count,), at, out = struct.unpack_from("<I", data, 8), 12, {}
    for _ in range(count):
        if len(data) - at < 8:
            raise ValueError("truncated entry")
        ident, length = struct.unpack_from("<II", data, at)
        at += 8
        if length > len(data) - at or b"\0" in data[at:at + length]:
            raise ValueError("bad text length")
        out[ident] = data[at:at + length]
        at += length
    if at != len(data):
        raise ValueError("trailing bytes")
    return out


def forbidden_entry(arcname: str) -> str | None:
    """Motivo para recusar um arquivo no zip do GitHub, ou None."""
    parts = arcname.lower().split("/")
    name = parts[-1]
    if "work" in parts[:-1]:
        return "generated game code (compiler/work)"
    if name == "castlevaniacod.dll" or name.endswith(".pdb"):
        return "recompiled game DLL or debug symbols"
    if ".private" in name or name == CATALOG_FILE:
        return "private PT-BR catalog"
    if Path(name).suffix in GAME_DATA:
        return "game data"
    if "assets" in parts[:-1] and name != "readme.txt":
        return "file inside assets/"
    return None


def leaked_texts(exe: bytes, catalog: dict, sample: int = 40) -> list:
    """Os textos PT-BR mais longos que aparecem no exe: vazio quando o catalogo nao foi compilado nele."""
    longest = sorted(catalog.values(), key=len, reverse=True)[:sample]
    return [text for text in longest if len(text) >= 24 and text in exe]


def exe_problems(exe: bytes, catalog: dict) -> list:
    """Motivos para nao publicar o exe: texto do catalogo, caminho da maquina ou amostra pequena demais."""
    samples = [text for text in sorted(catalog.values(), key=len, reverse=True)[:40] if len(text) >= 24]
    problems = []
    if len(samples) < MIN_LEAK_SAMPLES:
        problems.append(f"catalog has only {len(samples)} long texts to look for (needs {MIN_LEAK_SAMPLES})")
    if leaks := leaked_texts(exe, catalog):
        problems.append(f"{len(leaks)} PT-BR catalog texts inside: build it with -DCOD_PUBLIC_BUILD=ON")
    if path := PRIVATE_PATH.search(exe):
        problems.append(f"machine path inside: {path.group(0).decode(errors='replace')}...")
    return problems


def compiler_entries(compiler: Path) -> dict:
    """Arquivos da pasta compiler que vao no zip (lista fechada, sem work/); sys.exit se faltar algum."""
    entries = {f"compiler/{name}": compiler / name for name in COMPILER_FILES}
    for folder in COMPILER_DIRS:
        files = sorted(path for path in (compiler / folder).rglob("*") if path.is_file())
        if not files:
            sys.exit(f"compiler folder incomplete: {compiler / folder} is empty or missing")
        for path in files:
            entries[f"compiler/{path.relative_to(compiler).as_posix()}"] = path
    if not any((compiler / "config").glob("*.toml")):
        sys.exit(f"compiler folder incomplete: no config/*.toml in {compiler}")
    return entries


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def write_sums(out: Path, archives: list) -> None:
    sums = out / "SHA256SUMS.txt"
    lines = {}
    if sums.exists():
        for line in sums.read_text(encoding="utf-8").splitlines():
            digest, _, name = line.partition("  ")
            lines[name] = digest
    for archive in archives:
        lines[archive.name] = sha256(archive)
    sums.write_text("".join(f"{lines[name]}  {name}\n" for name in sorted(lines)), encoding="utf-8")


def vc_runtime_dir() -> Path:
    vswhere = Path(r"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")
    install = subprocess.run([str(vswhere), "-latest", "-products", "*", "-property", "installationPath"],
                             check=True, capture_output=True, text=True).stdout.strip()
    found = sorted(Path(install, "VC", "Redist", "MSVC").glob("1*/x64/Microsoft.VC*.CRT"))
    if not found or not (found[-1] / "vcruntime140.dll").exists():
        sys.exit("Visual C++ runtime not found (Visual Studio with the C++ workload)")
    return found[-1]


def sdk_dir(build: Path) -> Path:
    # o configure do SDK sempre grava sdk_dir.txt, ache ele o SDK pela variavel ou pelas pastas vizinhas
    marker = build / "sdk_dir.txt"
    sdk = Path(marker.read_text(encoding="utf-8").strip()) if marker.is_file() else None
    if not sdk or not (sdk / "THIRD_PARTY.txt").is_file():
        sys.exit(f"SDK not found through {marker}: reconfigure the build")
    return sdk


def kit(args) -> Path:
    build = Path(args.build)
    cache = (build / "CMakeCache.txt").read_text(encoding="utf-8")
    if "COD_PUBLIC_BUILD:BOOL=ON" not in cache:
        sys.exit(f"{build} is not a public build: configure it with -DCOD_PUBLIC_BUILD=ON")
    exe = build / EXE
    vc = vc_runtime_dir()
    entries = {EXE: exe}
    for name in VC_RUNTIME:
        entries[name] = vc / name
    entries.update(compiler_entries(build / "compiler"))
    for name in ("msvcp140.dll", "vcruntime140.dll"):  # xbe_recomp.exe procura ao lado dele
        entries[f"compiler/{name}"] = vc / name
    for name in DOCS:
        entries[name] = ROOT / name
    for path in sorted((ROOT / "LICENSES").glob("*")):
        entries[f"LICENSES/{path.name}"] = path
    entries["THIRD_PARTY.txt"] = sdk_dir(build) / "THIRD_PARTY.txt"
    texts = {"README.txt": KIT_README, "assets/README.txt": ASSETS_README, "mods/README.txt": MODS_README}

    refused = [(name, why) for name in [*entries, *texts] if (why := forbidden_entry(name))]
    if refused:
        sys.exit("refused: " + "; ".join(f"{name} ({why})" for name, why in refused))
    missing = [str(path) for path in entries.values() if not path.is_file()]
    if missing:
        sys.exit("missing: " + ", ".join(missing))
    if problems := exe_problems(exe.read_bytes(), parse_catalog(Path(args.catalog).read_bytes())):
        sys.exit(f"{EXE}: " + "; ".join(problems))

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    archive = out / f"{TOP}-{args.version}-win64.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zipped:
        for name, path in entries.items():
            zipped.write(path, f"{TOP}/{name}")
        for name, text in texts.items():
            zipped.writestr(f"{TOP}/{name}", crlf(text))
    with zipfile.ZipFile(archive) as zipped:
        bad = zipped.testzip()
        if bad:
            sys.exit(f"CRC error in {bad}")
    write_sums(out, [archive])
    write_folder_readme(out)
    print(f"{archive.name}: {len(entries) + len(texts)} files, {archive.stat().st_size / 2**20:.0f} MB")
    return archive


def packs(args) -> list:
    mods, out = Path(args.mods), Path(args.out)
    catalog = Path(args.catalog).read_bytes()
    count = len(parse_catalog(catalog))
    out.mkdir(parents=True, exist_ok=True)
    archives = []
    for pack in args.only or PACKS:
        folder = mods / pack
        files = sorted(path for path in folder.rglob("*") if path.is_file())
        if not files:
            sys.exit(f"empty pack: {folder}")
        archive = out / f"{TOP}-{pack}.zip"
        expected = {}
        with zipfile.ZipFile(archive, "w", allowZip64=True) as zipped:
            for path in files:
                name = f"mods/{pack}/{path.relative_to(folder).as_posix()}"
                if path == folder / "README.txt" or pack == "PTBR-PS2" and path.name == CATALOG_FILE:
                    continue  # README e catalogo entram abaixo, sempre os atuais
                # video ja comprimido: guardado; o resto comprime
                method = zipfile.ZIP_STORED if path.suffix.lower() == ".cv2" else zipfile.ZIP_DEFLATED
                zipped.write(path, name, compress_type=method, compresslevel=6)
                expected[name] = path.stat().st_size
            name = f"mods/{pack}/README.txt"
            readme = crlf(pack_readme(pack)).encode("utf-8")
            zipped.writestr(name, readme, compress_type=zipfile.ZIP_DEFLATED)
            expected[name] = len(readme)
            if pack == "PTBR-PS2":
                name = f"mods/{pack}/{CATALOG_FILE}"
                zipped.writestr(name, catalog, compress_type=zipfile.ZIP_DEFLATED)
                expected[name] = len(catalog)
        with zipfile.ZipFile(archive) as zipped:
            found = {info.filename: info.file_size for info in zipped.infolist()}
            if found != expected:
                sys.exit(f"{archive.name}: zip contents differ from {folder}")
            if bad := zipped.testzip():
                sys.exit(f"{archive.name}: CRC error in {bad}")
        extra = f", catalog with {count} texts" if pack == "PTBR-PS2" else ""
        print(f"{archive.name}: {len(expected)} files, {archive.stat().st_size / 2**20:.0f} MB{extra}")
        archives.append(archive)
    write_sums(out, archives)
    write_folder_readme(out)
    return archives


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    k = sub.add_parser("kit")
    k.add_argument("--build", required=True)
    k.add_argument("--version", required=True)
    k.add_argument("--out", required=True)
    k.add_argument("--catalog", required=True, help="ptbr_strings.bin: its texts must not be in the exe")
    p = sub.add_parser("packs")
    p.add_argument("--mods", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--catalog", required=True)
    p.add_argument("--only", nargs="*", choices=PACKS)
    args = parser.parse_args()
    kit(args) if args.command == "kit" else packs(args)


if __name__ == "__main__":
    main()
