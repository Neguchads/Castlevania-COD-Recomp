import struct
import tempfile
import unittest
import zipfile
from argparse import Namespace
from pathlib import Path

from release_kit import compiler_entries, exe_problems, forbidden_entry, leaked_texts, packs, parse_catalog, sdk_dir

# os mesmos bytes de tools/test_ptbr_catalog.cpp, gravados por SerializeCatalog do C++
GOLDEN = bytes.fromhex("434f445054425231" "02000000" "01000000" "02000000" "6162" "07000000" "04000000" "4d617061")


def catalog(entries):
    data = b"CODPTBR1" + struct.pack("<I", len(entries))
    for ident, text in sorted(entries.items()):
        data += struct.pack("<II", ident, len(text)) + text
    return data


class ReleaseKitTests(unittest.TestCase):
    def test_catalog_format_matches_the_cpp_reader(self):
        entries = {7: b"Mapa", 0: b"", 1409: "Olá\x01 mundo".encode()}
        self.assertEqual(parse_catalog(catalog(entries)), entries)
        good = catalog({1: b"ab"})
        for bad in (b"", b"XODPTBR1" + good[8:], good[:-1], good + b"\0",
                    good[:12] + struct.pack("<II", 1, 99) + b"ab", catalog({2: b"a\0b"})):
            with self.assertRaises(ValueError):
                parse_catalog(bad)

    def test_golden_bytes_from_the_cpp_writer(self):
        self.assertEqual(parse_catalog(GOLDEN), {1: b"ab", 7: b"Mapa"})

    def test_github_zip_refuses_game_data_and_generated_code(self):
        for name in ("CastlevaniaCOD.dll", "compiler/work/CastlevaniaCOD/x86_recomp.0.obj",
                     "compiler/work/game/x86_recomp.0.cpp", "assets/default.xbe", "assets/game.iso",
                     "assets/ptbr_strings.private.h", "mods/PTBR-PS2/ptbr_strings.bin",
                     "mods/HD4K/textures/a.dds", "CastlevaniaCOD.pdb", "assets/anything.txt"):
            self.assertIsNotNone(forbidden_entry(name), name)
        for name in ("CastlevaniaCOD.exe", "compiler/xbe_recomp.exe", "compiler/clang.exe",
                     "compiler/config/game.toml", "compiler/include/cpu/x86_ops.h", "compiler/vcruntime140.dll",
                     "README.txt", "assets/README.txt", "mods/README.txt", "LICENSES/XboxXBERecomp-MIT.txt", "README.md"):
            self.assertIsNone(forbidden_entry(name), name)

    def test_catalog_texts_in_the_exe_are_reported(self):
        texts = {1: b"Um texto comprido o bastante para contar", 2: b"curto"}
        self.assertEqual(leaked_texts(b"MZ...Um texto comprido o bastante para contar...", texts),
                         [texts[1]])
        self.assertEqual(leaked_texts(b"MZ... sem catalogo, so curto ...", texts), [])

    def test_exe_check_needs_enough_samples_and_finds_machine_paths(self):
        texts = {i: f"texto numero {i:03d} comprido o bastante".encode() for i in range(30)}
        self.assertEqual(exe_problems(b"MZ limpo C:/Windows/USB Vibration", texts), [])
        self.assertEqual(len(exe_problems(b"MZ " + texts[5], texts)), 1)
        # montado em partes: o teste de higiene recusa caminho de maquina escrito no codigo
        for path in (rb"\Recomps e Mods\x.pdb", rb"\Users\nome\x", b"/Users/nome/x"):
            self.assertIn("machine path", exe_problems(b"MZ G:" + path, texts)[0])
        self.assertIn("only 1 long texts", exe_problems(b"MZ", {1: b"um texto comprido o bastante aqui"})[0])

    def test_compiler_folder_is_an_allowlist_without_work(self):
        with tempfile.TemporaryDirectory() as temp:
            compiler = Path(temp)
            for name in ("clang.exe", "lld-link.exe", "xbe_recomp.exe", "LLVM-LICENSE.txt", "CastlevaniaCOD.lib",
                         "config/game.toml", "include/cpu/x86_ops.h", "xdk/5849/layout.toml",
                         "work/CastlevaniaCOD/x86_recomp.0.cpp", "build.log", "msvcp140.dll"):
                (compiler / name).parent.mkdir(parents=True, exist_ok=True)
                (compiler / name).write_text("x")
            names = set(compiler_entries(compiler))
            self.assertIn("compiler/config/game.toml", names)
            self.assertIn("compiler/xdk/5849/layout.toml", names)
            self.assertFalse({"compiler/build.log", "compiler/msvcp140.dll"} & names)
            self.assertFalse(any("/work/" in name for name in names))
            for name in ("xdk/5849/layout.toml", "config/game.toml"):
                (compiler / name).unlink()
                with self.assertRaises(SystemExit):
                    compiler_entries(compiler)
                (compiler / name).write_text("x")

    def test_sdk_comes_from_the_marker_the_configure_writes(self):
        with tempfile.TemporaryDirectory() as temp:
            build, sdk = Path(temp, "build"), Path(temp, "sdk")
            build.mkdir()
            sdk.mkdir()
            with self.assertRaises(SystemExit):
                sdk_dir(build)
            (build / "sdk_dir.txt").write_text(f"{sdk}\n")
            with self.assertRaises(SystemExit):
                sdk_dir(build)
            (sdk / "THIRD_PARTY.txt").write_text("x")
            self.assertEqual(sdk_dir(build), sdk)

    def test_pack_zips_keep_the_mods_layout_and_add_the_catalog(self):
        with tempfile.TemporaryDirectory() as temp:
            mods, out = Path(temp, "mods"), Path(temp, "out")
            for name in ("PTBR-PS2/files/data/a.dar", "PTBR-PS2/ptbr_strings.bin", "PS2-Botoes/textures/b.dds"):
                (mods / name).parent.mkdir(parents=True, exist_ok=True)
                (mods / name).write_bytes(b"velho")
            catalog = Path(temp, "ptbr_strings.bin")
            catalog.write_bytes(GOLDEN)
            packs(Namespace(mods=str(mods), out=str(out), catalog=str(catalog), only=["PTBR-PS2", "PS2-Botoes"]))
            with zipfile.ZipFile(out / "Castlevania-COD-Recomp-PTBR-PS2.zip") as zipped:
                self.assertEqual(sorted(zipped.namelist()),
                                 ["mods/PTBR-PS2/README.txt", "mods/PTBR-PS2/files/data/a.dar",
                                  "mods/PTBR-PS2/ptbr_strings.bin"])
                readme = zipped.read("mods/PTBR-PS2/README.txt").decode("utf-8")
                self.assertLess(readme.index("ENGLISH"), readme.index("PORTUGUES (BRASIL)"))
                self.assertIn("\r\n", readme)
                self.assertEqual(zipped.read("mods/PTBR-PS2/ptbr_strings.bin"), GOLDEN)
            with zipfile.ZipFile(out / "Castlevania-COD-Recomp-PS2-Botoes.zip") as zipped:
                self.assertEqual(sorted(zipped.namelist()),
                                 ["mods/PS2-Botoes/README.txt", "mods/PS2-Botoes/textures/b.dds"])
            self.assertIn("ENGLISH", (out / "README.txt").read_text(encoding="utf-8"))
            sums = (out / "SHA256SUMS.txt").read_text().splitlines()
            # o README da pasta de saida nao entra nos SHA256 (so os zips)
            self.assertEqual([line.split("  ")[1] for line in sums],
                             ["Castlevania-COD-Recomp-PS2-Botoes.zip", "Castlevania-COD-Recomp-PTBR-PS2.zip"])


class ReadmeTextTests(unittest.TestCase):
    def test_docs_named_in_the_readmes_ship_in_the_zip(self):
        import re as regex
        from release_kit import DOCS, KIT_README, PACKS, pack_readme
        named = set()
        for text in (KIT_README, *map(pack_readme, PACKS)):
            named |= {name.replace(chr(92), "/") for name in regex.findall(r"(?:docs\\)?[A-Z]+\.md", text)}
        self.assertTrue({"docs/GUIDE.md", "MODDING.md", "docs/CREDITS.md"} <= named, named)
        self.assertEqual(named - set(DOCS), set())

    def test_every_text_for_players_is_english_then_portuguese(self):
        from release_kit import ASSETS_README, KIT_README, MODS_README, PACKS, RELEASE_FOLDER_README, pack_readme
        for text in (KIT_README, ASSETS_README, MODS_README, RELEASE_FOLDER_README, *map(pack_readme, PACKS)):
            self.assertLess(text.index("ENGLISH"), text.index("PORTUGUES (BRASIL)"))
            self.assertTrue(text.isascii(), text[:40])  # abre igual em qualquer Bloco de Notas


if __name__ == "__main__":
    unittest.main()
