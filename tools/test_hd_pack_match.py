import json
import struct
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

import numpy as np
from PIL import Image

try:
    import hd_pack_match as hm
    from dar_texture import entry_rgba, pack_hash
    from ps2_ptbr_import import DAR_MAGIC
    from test_dar_texture import p8_entry
except ImportError:
    from tools import hd_pack_match as hm
    from tools.dar_texture import entry_rgba, pack_hash
    from tools.ps2_ptbr_import import DAR_MAGIC
    from tools.test_dar_texture import p8_entry

ROOT = Path(__file__).resolve().parents[1]
TEXCONV = ROOT / '.local/build-tools/texconv.exe'


def dar(entries: list[bytes]) -> bytes:
    """DAR mínimo: cabeçalho, registros de 16 bytes e entradas (offset relativo ao registro)."""
    head = bytearray(0x20 + 16 * len(entries))
    head[:4] = DAR_MAGIC
    struct.pack_into('<I', head, 0x10, len(entries))
    body = bytearray()
    for i, entry in enumerate(entries):
        rec = 0x20 + 16 * i
        struct.pack_into('<IIII', head, rec, 100 + i, hm.TEXTURE, len(head) + len(body) - rec, len(entry))
        body += entry
    return bytes(head + body)


def upscale(rgba: np.ndarray, factor: int, noise: int = 0, seed: int = 0) -> np.ndarray:
    big = np.repeat(np.repeat(rgba, factor, 0), factor, 1).astype(np.int16)
    if noise:
        big += np.random.default_rng(seed).integers(-noise, noise + 1, big.shape, dtype=np.int16)
    return np.clip(big, 0, 255).astype(np.uint8)


def png_item(name: str, rgba: np.ndarray) -> tuple:
    h, w = rgba.shape[:2]
    return (name, w, h, hm.signature(rgba, hm.COARSE).tolist(), hm.signature(rgba, hm.FINE).tolist())


class HdPackMatchTests(unittest.TestCase):
    def setUp(self):
        self.a = entry_rgba(p8_entry(8, 8, seed=1))
        self.b = entry_rgba(p8_entry(8, 8, seed=2))
        self.tall = entry_rgba(p8_entry(8, 16, seed=3))
        self.xbox = {
            1: {'dar': 'a.dar', 'id': 1, 'w': 8, 'h': 8, 'coarse': hm.signature(self.a, hm.COARSE),
                'fine': hm.signature(self.a, hm.FINE)},
            2: {'dar': 'a.dar', 'id': 2, 'w': 8, 'h': 8, 'coarse': hm.signature(self.b, hm.COARSE),
                'fine': hm.signature(self.b, hm.FINE)},
            3: {'dar': 'a.dar', 'id': 3, 'w': 8, 'h': 16, 'coarse': hm.signature(self.tall, hm.COARSE),
                'fine': hm.signature(self.tall, hm.FINE)},
        }

    def test_upscaled_png_matches_its_xbox_texture(self):
        pngs = [png_item('a4k.png', upscale(self.a, 4, noise=6)), png_item('b4k.png', upscale(self.b, 4, noise=6))]
        result = hm.match(self.xbox, pngs)
        self.assertEqual(result['pares']['0000000000000001']['png'], 'a4k.png')
        self.assertEqual(result['pares']['0000000000000002']['png'], 'b4k.png')
        # 8x16 não tem PNG na mesma proporção: não pode receber um quadrado
        self.assertEqual(result['rejeitados']['0000000000000003']['motivo'], 'sem PNG na mesma proporção')
        self.assertEqual(result['png_sem_par'], [])

    def test_unrelated_or_ambiguous_png_is_rejected(self):
        other = entry_rgba(p8_entry(8, 8, seed=9))
        result = hm.match({1: self.xbox[1]}, [png_item('x.png', upscale(other, 4))])
        self.assertNotIn('0000000000000001', result['pares'])
        self.assertEqual(result['png_sem_par'], ['x.png'])

    def test_local_difference_and_true_ambiguity_are_rejected(self):
        # 32x32 opaco: a assinatura FINE é a própria imagem, então os blocos 4x4 são exatos
        base = np.random.default_rng(3).integers(40, 200, (32, 32, 4)).astype(np.uint8)
        base[..., 3] = 255
        tex = {1: {'dar': 'a.dar', 'id': 1, 'w': 32, 'h': 32, 'coarse': hm.signature(base, hm.COARSE),
                   'fine': hm.signature(base, hm.FINE)}}
        def changed(delta):
            out = base.astype(np.int16)
            out[:4, :4, :3] += delta   # um bloco só: a média quase não muda
            return np.clip(out, 0, 255).astype(np.uint8)
        # outro conteúdo num canto (monstro diferente na mesma moldura): pior bloco reprova
        result = hm.match(tex, [png_item('canto.png', changed(40))])
        self.assertEqual(result['rejeitados']['0000000000000001']['motivo'], 'imagem diferente')
        # duas variantes plausíveis e diferentes entre si (cor do olho): ambígua
        result = hm.match(tex, [png_item('mais.png', changed(15)), png_item('menos.png', changed(-15))])
        self.assertEqual(result['rejeitados']['0000000000000001']['motivo'], 'ambígua')
        # variantes quase iguais entre si não são rivais
        result = hm.match(tex, [png_item('a.png', changed(3)), png_item('b.png', changed(-3))])
        self.assertIn('0000000000000001', result['pares'])

    def test_identical_pngs_do_not_count_as_ambiguous(self):
        pngs = [png_item('a1.png', upscale(self.a, 2)), png_item('a2.png', upscale(self.a, 2))]
        self.assertIn('0000000000000001', hm.match({1: self.xbox[1]}, pngs)['pares'])

    def test_xbox_scan_uses_runtime_hash_and_dedupes(self):
        entry, other = p8_entry(8, 8, seed=1), p8_entry(8, 4, seed=4)
        external = bytearray(p8_entry(8, 8, seed=5))
        external[-1024:] = bytes(1024)   # paleta externa: hash depende do jogo, fica de fora
        with tempfile.TemporaryDirectory() as tmp:
            (Path(tmp) / 'x.dar').write_bytes(dar([entry, other, bytes(external)]))
            (Path(tmp) / 'sub').mkdir()
            (Path(tmp) / 'sub/y.dar').write_bytes(dar([entry]))
            found = hm.xbox_textures(Path(tmp))
        self.assertEqual(set(found), {pack_hash(entry), pack_hash(other)})
        self.assertEqual((found[pack_hash(other)]['w'], found[pack_hash(other)]['h']), (8, 4))

    def test_png_signature_reads_size_reduces_and_converts_ps2_alpha(self):
        ps2 = upscale(self.a, 32)
        ps2[..., 3] = (ps2[..., 3].astype(np.uint16) + 1) // 2   # PCSX2: 0x80 = opaco
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 't.png'
            Image.fromarray(ps2, 'RGBA').save(path)
            name, w, h, _, fine = hm.png_signature(str(path))
        self.assertEqual((name, w, h), ('t.png', 256, 256))
        self.assertLess(np.abs(np.array(fine) - self.xbox[1]['fine']).mean(), 2.0)

    def test_ps2_alpha_scale(self):
        rgba = np.array([[[1, 2, 3, 0], [4, 5, 6, 64], [7, 8, 9, 128], [1, 1, 1, 200]]], np.uint8)
        out = hm.ps2_alpha_to_xbox(rgba)
        self.assertEqual(out[0, :, 3].tolist(), [0, 128, 255, 255])
        self.assertEqual(out[..., :3].tolist(), rgba[..., :3].tolist())

    def test_translation_keeps_english_4k_and_replaces_only_the_text(self):
        english = np.zeros((16, 16, 4), np.uint8)
        english[..., :3], english[..., 3] = 90, 255
        translated = english.copy()
        translated[2:4, 2:6, :3] = 250                     # "texto" PT-BR num canto
        english_4k = upscale(english, 8, noise=20, seed=5)  # detalhe que só o 4K tem
        out, area = hm.compose_translation(english_4k, english, translated)
        self.assertEqual(out.shape, english_4k.shape)
        self.assertAlmostEqual(area, 8 / 256)
        # longe do texto: exatamente o 4K inglês
        self.assertTrue(np.array_equal(out[80:, 80:], english_4k[80:, 80:]))
        # no meio do texto: a cor da PT-BR
        self.assertGreater(out[24, 32, :3].min(), 230)

    def test_near_keeps_4k_where_equal_and_original_where_different(self):
        xbox = np.zeros((16, 16, 4), np.uint8)
        xbox[..., :3], xbox[..., 3] = 90, 255
        pack_4k = upscale(xbox, 8, noise=20, seed=5)   # mesma imagem com detalhe que só o 4K tem
        same, area = hm.compose_near(pack_4k, xbox)
        self.assertEqual(area, 0.0)
        # detalhe do 4K preservado; só a cor média de cada pixel do Xbox vem da original
        self.assertLess(np.abs(same.astype(int) - pack_4k.astype(int)).mean(), 1.5)
        # ícone diferente num canto: ali fica a original, longe dali o 4K
        icon = pack_4k.copy()
        icon[:32, :32, :3] = 250
        out, area = hm.compose_near(icon, xbox)
        self.assertAlmostEqual(area, 16 / 256)
        self.assertLess(np.abs(out[8:24, 8:24, :3].astype(int) - 90).max(), 3)
        self.assertLess(np.abs(out[80:, 80:].astype(int) - icon[80:, 80:].astype(int)).mean(), 1.5)

    def test_near_does_not_copy_faint_tint_of_another_image(self):
        # aura fraca de outro monstro: abaixo do limite, mas a cor média tem de ser a da original
        xbox = np.zeros((16, 16, 4), np.uint8)
        xbox[..., 3] = 255
        ghost = upscale(xbox, 8)
        ghost[:64, :64, 0] = 20
        out, area = hm.compose_near(ghost, xbox)
        self.assertEqual(area, 0.0)
        self.assertLess(out[8:56, 8:56, 0].mean(), 2)

    def test_near_writes_report_and_skips_too_different(self):
        entries = [p8_entry(8, 8, seed=1), p8_entry(8, 8, seed=2)]
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            (tmp / 'assets').mkdir()
            (tmp / 'assets/a.dar').write_bytes(dar(entries))
            close, other = entry_rgba(entries[0]), entry_rgba(entries[1])
            def ps2(rgba):   # o pacote grava o alpha do PS2 (0x80 = opaco)
                out = upscale(rgba, 4)
                out[..., 3] = (out[..., 3].astype(np.uint16) + 1) // 2
                return out
            Image.fromarray(ps2(close), 'RGBA').save(tmp / 'close.png')
            Image.fromarray(ps2(other), 'RGBA').save(tmp / 'other.png')
            rejected = {
                'AA': {'motivo': 'imagem diferente', 'png': 'close.png', 'distancia': 7.0, 'dar': 'a.dar', 'id': 100},
                'BB': {'motivo': 'imagem diferente', 'png': 'other.png', 'distancia': 9.0, 'dar': 'a.dar', 'id': 100},
                'CC': {'motivo': 'imagem diferente', 'png': 'close.png', 'distancia': 30.0, 'dar': 'a.dar', 'id': 100},
                'DD': {'motivo': 'ambígua', 'png': 'close.png', 'distancia': 2.0, 'dar': 'a.dar', 'id': 100}}
            (tmp / 'm.json').write_text(json.dumps({'rejeitados': rejected}))
            args = SimpleNamespace(match=tmp / 'm.json', pack=tmp, assets=tmp / 'assets', out=tmp / 'out',
                                   report=tmp / 'r.json', texconv=TEXCONV, max_distance=hm.NEAR_DISTANCE,
                                   max_area=hm.NEAR_MAX_AREA, preview=tmp / 'prev', dry_run=True)
            hm.near(args)
            report = {r['hash']: r for r in json.loads((tmp / 'r.json').read_text(encoding='utf-8'))}
            self.assertEqual(set(report), {'AA', 'BB'})   # longe demais e ambígua ficam de fora
            self.assertEqual(report['AA']['status'], 'aprovada (simulação)')
            self.assertEqual(report['BB']['status'], 'diferente demais (original mantida)')
            self.assertTrue((tmp / 'prev/AA.png').exists())
            self.assertFalse((tmp / 'out/AA.dds').exists())

    def test_compose_cells_takes_4k_only_inside_cells(self):
        xbox = np.zeros((8, 8, 4), np.uint8); xbox[...] = (10, 20, 30, 255)
        pack = np.zeros((32, 32, 4), np.uint8); pack[...] = (200, 100, 50, 255)
        out = hm.compose_cells(pack, xbox, [(2, 4, 4, 6)])
        self.assertEqual(out.shape, (32, 32, 4))
        self.assertTrue((out[16:24, 8:16] == (200, 100, 50, 255)).all())   # célula: 4K
        self.assertTrue((out[0:16] == (10, 20, 30, 255)).all())            # fora: original ampliada
        self.assertTrue((out[16:24, 0:8] == (10, 20, 30, 255)).all())
        with self.assertRaises(ValueError):
            hm.compose_cells(np.zeros((30, 32, 4), np.uint8), xbox, [])

    def test_ps2_button_cells_fit_the_atlas_without_overlap(self):
        cells = hm.PS2_BUTTON_CELLS
        self.assertEqual(len(cells), 9)
        for i, (x0, y0, x1, y1) in enumerate(cells):
            self.assertTrue(0 <= x0 < x1 <= 256 and 0 <= y0 < y1 <= 256)
            for a0, b0, a1, b1 in cells[i + 1:]:
                self.assertTrue(x1 <= a0 or a1 <= x0 or y1 <= b0 or b1 <= y0)

    def buttons_case(self, tmp: Path, record: dict | None, key: str | None = None):
        """Atlas sintético 8x8 no lugar do global.dar#16020; devolve o código de saída e a pasta de saída."""
        entry = p8_entry(8, 8, seed=1)
        (tmp / 'g.dar').write_bytes(dar([entry]))
        Image.fromarray(upscale(entry_rgba(entry), 4), 'RGBA').save(tmp / 'a.png')
        key = key or f'{pack_hash(entry):016X}'
        (tmp / 'm.json').write_text(json.dumps({'pares': {}, 'rejeitados': {key: record} if record else {}}))
        args = SimpleNamespace(match=tmp / 'm.json', pack=tmp, assets=tmp, out=tmp / 'out', texconv=TEXCONV, preview=None)
        with mock.patch.object(hm, 'PS2_BUTTONS_DAR', ('g.dar', 100)), \
             mock.patch.object(hm, 'PS2_BUTTONS_HASH', f'{pack_hash(entry):016X}'), \
             mock.patch.object(hm, 'PS2_BUTTON_CELLS', ((0, 0, 4, 4),)):
            return hm.buttons(args), tmp / 'out'

    def test_buttons_refuses_missing_record_png_or_other_atlas(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            self.assertEqual(self.buttons_case(tmp, None)[0], 1)
            self.assertEqual(self.buttons_case(tmp, {'motivo': 'sem PNG na mesma proporção'})[0], 1)
            entry = p8_entry(8, 8, seed=1)
            self.assertEqual(hm.pack_hash_of(tmp, 'g.dar', 100), pack_hash(entry))
            self.assertIsNone(hm.pack_hash_of(tmp, 'g.dar', 999))
            with mock.patch.object(hm, 'pack_hash_of', return_value=0):
                self.assertEqual(self.buttons_case(tmp, {'png': 'a.png'})[0], 1)
            self.assertFalse((tmp / 'out').exists() and any((tmp / 'out').iterdir()))

    @unittest.skipUnless(TEXCONV.exists(), 'texconv local ausente')
    def test_buttons_writes_dds_named_by_atlas_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            code, out = self.buttons_case(tmp, {'png': 'a.png'})
            self.assertEqual(code, 0)
            files = list(out.glob('*.dds'))
            self.assertEqual(len(files), 1)
            self.assertEqual(files[0].stem, f'{pack_hash(p8_entry(8, 8, seed=1)):016X}')
            self.assertEqual(files[0].read_bytes()[:4], b'DDS ')

    def test_texconv_command_is_bc7_with_mips_and_no_gamma_change(self):
        cmd = hm.texconv_command(Path('t.exe'), Path('a.png'), Path('o'))
        self.assertIn('BC7_UNORM', cmd)
        self.assertEqual(cmd[cmd.index('-m') + 1], '0')
        self.assertIn('--ignore-srgb', cmd)

    @unittest.skipUnless(TEXCONV.exists(), 'texconv local ausente')
    def test_build_writes_bc7_dds_for_every_hash(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            Image.fromarray(upscale(self.a, 8), 'RGBA').save(tmp / 'a.png')
            pares = {'00000000000000AA': {'png': 'a.png'}, '00000000000000BB': {'png': 'a.png'}}
            (tmp / 'm.json').write_text(json.dumps({'pares': pares}))
            hm.build(SimpleNamespace(match=tmp / 'm.json', pack=tmp, out=tmp / 'out', texconv=TEXCONV))
            for key in pares:
                data = (tmp / 'out' / f'{key}.dds').read_bytes()
                self.assertEqual((data[:4], data[84:88]), (b'DDS ', b'DX10'))
                height, width = struct.unpack_from('<II', data, 12)
                levels, = struct.unpack_from('<I', data, 28)
                fmt, = struct.unpack_from('<I', data, 128)
                self.assertEqual((width, height, levels, fmt), (64, 64, 7, 98))


if __name__ == '__main__':
    unittest.main()
