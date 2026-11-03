import struct
import unittest
from pathlib import Path

import numpy as np

try:
    import dar_texture as dt
    import ps2_texture_import as tex
except ImportError:
    from tools import dar_texture as dt
    from tools import ps2_texture_import as tex

ROOT = Path(__file__).resolve().parents[1]


def p8_entry(w=8, h=4, seed=0):
    """Entrada Xbox P8 mínima: formato em +0x4C, pixels Morton em +0x100, paleta BGRA."""
    entry = bytearray(tex.XBOX_PIXELS + w * h + 1024)
    fmt = tex.FORMAT_P8 << 8 | (w.bit_length() - 1) << 20 | (h.bit_length() - 1) << 24
    struct.pack_into('<I', entry, tex.XBOX_D3D + 12, fmt)
    rng = np.random.default_rng(seed)
    palette = rng.integers(0, 256, (256, 4), dtype=np.uint8)
    entry[tex.XBOX_PIXELS + w * h:] = palette.tobytes()
    indices = rng.integers(0, 256, (h, w), dtype=np.uint8)
    return tex.write_xbox(bytes(entry), indices, None)


class DarTextureTests(unittest.TestCase):
    def test_unchanged_image_keeps_bytes(self):
        entry = p8_entry()
        new, repalette = dt.entry_with_rgba(entry, dt.entry_rgba(entry))
        self.assertFalse(repalette)
        self.assertEqual(dt.entry_rgba(new).tobytes(), dt.entry_rgba(entry).tobytes())

    def test_new_colors_rebuild_palette(self):
        entry = p8_entry()
        rgba = dt.entry_rgba(entry).copy()
        rgba[0, :4] = [1, 2, 3, 255]  # cor fora da paleta
        if dt.exact_indices(rgba, dt.entry_rgba(entry).reshape(-1, 4)) is not None:
            self.skipTest('cor sorteada já existe')
        new, repalette = dt.entry_with_rgba(entry, rgba)
        self.assertTrue(repalette)
        self.assertEqual(len(new), len(entry))
        self.assertEqual(new[:tex.XBOX_PIXELS], entry[:tex.XBOX_PIXELS])  # cabeçalho intacto
        self.assertLessEqual(np.abs(dt.entry_rgba(new)[0, :4].astype(int) - [1, 2, 3, 255]).max(), 8)

    def test_external_palette_is_rejected(self):
        entry = bytearray(p8_entry())
        entry[-1024:] = bytes(1024)  # CLUT externa: área de paleta vazia
        with self.assertRaises(ValueError):
            dt.entry_rgba(bytes(entry))
        with self.assertRaises(ValueError):
            dt.entry_with_rgba(bytes(entry), np.zeros((4, 8, 4), np.uint8))

    def test_bmp32_is_top_down_bgra(self):
        import tempfile
        rgba = np.array([[[1, 2, 3, 4], [5, 6, 7, 8]]], np.uint8)
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'x.bmp'
            dt.write_bmp32(path, rgba)
            data = path.read_bytes()
        width, height, planes, bits = struct.unpack_from('<iiHH', data, 18)
        self.assertEqual((width, height, planes, bits), (2, -1, 1, 32))
        self.assertEqual(data[54:], bytes([3, 2, 1, 4, 7, 6, 5, 8]))
        with self.assertRaises(ValueError):
            dt.write_bmp32(Path('nunca.bmp'), np.zeros((1, 16385, 4), np.uint8))

    @unittest.skipUnless((ROOT / 'assets/data/yama000.dar').exists(), 'assets locais ausentes')
    def test_pack_hash_matches_runtime_dump_name(self):
        # 96A74C0D77F0023D.bmp foi gerado pelo runtime (F1 > Mods > Exportar texturas).
        blob = (ROOT / 'assets/data/yama000.dar').read_bytes()
        start, size = dt.texture_records(blob, 2299)[0]
        self.assertEqual(f'{dt.pack_hash(blob[start:start + size]):016X}', '96A74C0D77F0023D')

    def test_wrong_size_is_rejected(self):
        with self.assertRaises(ValueError):
            dt.entry_with_rgba(p8_entry(), np.zeros((8, 8, 4), np.uint8))

    @unittest.skipUnless((ROOT / 'out/build/release/mods/PTBR-PS2/files/data/titleue.dar').exists(),
                         'mod PTBR-PS2 local ausente')
    def test_real_title_background_roundtrip(self):
        blob = (ROOT / 'out/build/release/mods/PTBR-PS2/files/data/titleue.dar').read_bytes()
        start, size = dt.texture_records(blob, 14452)[0]
        entry = blob[start:start + size]
        new, repalette = dt.entry_with_rgba(entry, dt.entry_rgba(entry))
        self.assertFalse(repalette)
        self.assertEqual(new, entry)


if __name__ == '__main__':
    unittest.main()
