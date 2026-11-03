import struct
import unittest

import numpy as np

try:
    import ps2_texture_import as tex
except ModuleNotFoundError:
    from tools import ps2_texture_import as tex


def scalar_unswizzle8(x, y, w):
    """Formula escalar classica (decode_ps2_tex.py), conferida na tela-titulo."""
    block = (y & ~0xF) * w + (x & ~0xF) * 2
    swap = (((y + 2) >> 2) & 1) * 4
    py = (((y & ~3) >> 1) + (y & 1)) & 7
    column = py * w * 2 + ((x + swap) & 7) * 4
    return block + column + ((y >> 1) & 1) + ((x >> 2) & 2)


def scalar_morton(x, y, w, h):
    """Deposit do SDK (runtime/gpu/texture.cpp): bits alternados, x primeiro."""
    out, bit, i = 0, 0, 0
    lx, ly = w.bit_length() - 1, h.bit_length() - 1
    while i < max(lx, ly):
        if i < lx:
            out |= ((x >> i) & 1) << bit
            bit += 1
        if i < ly:
            out |= ((y >> i) & 1) << bit
            bit += 1
        i += 1
    return out


def ps2_entry(w, h, psm, pixels, clut=None):
    head = bytearray(0x80)
    pix = 0x80
    clut_off = pix + len(pixels) if clut is not None else pix
    struct.pack_into('<HHHH', head, 0x10, pix, clut_off, w, h)
    head[0x19] = psm
    return bytes(head) + pixels + (clut or b'')


def xbox_entry(w, h, pixels=None, palette=None):
    head = bytearray(tex.XBOX_PIXELS)
    fmt = (tex.FORMAT_P8 << 8) | ((w.bit_length() - 1) << 20) | ((h.bit_length() - 1) << 24)
    struct.pack_into('<I', head, tex.XBOX_D3D + 12, fmt)
    return bytes(head) + (pixels or bytes(w * h)) + (palette or bytes(1024))


class LayoutTests(unittest.TestCase):
    def test_vector_maps_match_scalar_formulas(self):
        for w, h in ((32, 16), (64, 64), (128, 32)):
            umap, mmap = tex.unswizzle8_map(w, h), tex.morton_map(w, h)
            for y in range(h):
                for x in range(w):
                    self.assertEqual(umap[y, x], scalar_unswizzle8(x, y, w))
                    self.assertEqual(mmap[y, x], scalar_morton(x, y, w, h))

    def test_psmt4_expands_low_nibble_first_then_unswizzles(self):
        w, h = 32, 16
        linear = (np.arange(w * h) % 16).astype(np.uint8)
        packed = (linear[0::2] | linear[1::2] << 4).astype(np.uint8).tobytes()
        indices = tex.ps2_indices(ps2_entry(w, h, tex.PSMT4, packed))
        self.assertTrue(np.array_equal(indices, linear[tex.unswizzle8_map(w, h)]))

    def test_psmt8_palette_uses_csm1_and_external_clut_returns_none(self):
        clut = np.zeros((256, 4), np.uint8)
        clut[:, 0] = np.arange(256)
        entry = ps2_entry(16, 16, tex.PSMT8, bytes(256), clut.tobytes())
        palette = tex.ps2_palette(entry)
        self.assertEqual(palette[8, 0], 16)   # 8..15 <-> 16..23 em cada bloco de 32
        self.assertEqual(palette[16, 0], 8)
        self.assertEqual(palette[40, 0], 48)
        self.assertIsNone(tex.ps2_palette(ps2_entry(16, 16, tex.PSMT8, bytes(256))))


class XboxTests(unittest.TestCase):
    def test_palette_conversion_bgra_alpha_and_untouched_tail(self):
        template = np.full((256, 4), 7, np.uint8)
        rgba = np.array([[10, 20, 30, 0x80], [1, 2, 3, 0x40]], np.uint8)
        out = tex.xbox_palette_from_ps2(rgba, template)
        self.assertEqual(out[0].tolist(), [30, 20, 10, 255])
        self.assertEqual(out[1].tolist(), [3, 2, 1, 128])
        self.assertEqual(out[2].tolist(), [7, 7, 7, 7])

    def test_write_keeps_padding_rows_and_writes_palette(self):
        w, h = 16, 16
        original = xbox_entry(w, h, bytes([9]) * (w * h))
        indices = np.arange(w * 12, dtype=np.uint8).reshape(12, w)  # 16x12 dentro de 16x16
        palette = np.full((256, 4), 5, np.uint8)
        out = tex.write_xbox(original, indices, palette)
        decoded = tex.xbox_indices(out)
        self.assertTrue(np.array_equal(decoded[:12], indices))
        self.assertTrue((decoded[12:] == 9).all())
        self.assertTrue((tex.xbox_palette(out) == 5).all())
        self.assertEqual(tex.write_xbox(original, indices, None)[-1024:], original[-1024:])
        with self.assertRaises(ValueError):
            tex.write_xbox(original, np.zeros((32, 16), np.uint8), None)

    def test_non_p8_entry_is_rejected(self):
        entry = bytearray(xbox_entry(16, 16))
        struct.pack_into('<I', entry, tex.XBOX_D3D + 12, 0x06 << 8 | 4 << 20 | 4 << 24)
        with self.assertRaises(ValueError):
            tex.xbox_layout(bytes(entry))


class PairingTests(unittest.TestCase):
    def setUp(self):
        rng = np.random.default_rng(7)
        self.w = self.h = 16
        self.background = rng.integers(0, 16, (16, 16), dtype=np.uint8)

    def ps2(self, indices):
        linear = np.zeros(self.w * self.h, np.uint8)
        linear[tex.unswizzle8_map(self.w, self.h).reshape(-1)] = indices.reshape(-1)
        return ps2_entry(self.w, self.h, tex.PSMT8, linear.tobytes())

    def dar(self, entries):
        head = bytearray(0x20 + 16 * len(entries))
        struct.pack_into('<I', head, 0x10, len(entries))
        body = bytearray()
        for i, (eid, data) in enumerate(entries):
            rec = 0x20 + 16 * i
            struct.pack_into('<IIII', head, rec, eid, tex.TEXTURE, len(head) + len(body) - rec, len(data))
            body += data
        return bytes(head + body)

    def test_english_dar_suffix(self):
        self.assertTrue(tex.english_dar('titleue.dar'))
        self.assertFalse(tex.english_dar('title.dar'))
        self.assertFalse(tex.english_dar('etcareakepaee.dar'))  # traz ID japones

    def test_shared_id_global_tie_prefers_american_and_group_breaks_card_ties(self):
        spanish = self.background.copy()
        spanish[:2] = 15
        japanese = self.background.copy()
        english_a, english_b = self.background.copy(), self.background.copy()
        english_b[15, :4] = (english_b[15, :4] + 1) % 16  # mesmo fundo, outro cartao
        textures = {1: japanese, 2: english_a, 3: english_b}
        orig = [self.dar([(3, self.ps2(english_b))]), self.dar([(9, self.ps2(spanish))])]
        item = {'dar': 1, 'id': 9, 'orig': self.ps2(spanish)}
        self.assertEqual(tex.english_counterpart({**item, 'id': 1}, orig, {1}, set(), textures)[2], 'shared_id')
        chosen, _, method = tex.english_counterpart(item, orig, {1, 2, 3}, {2, 3}, textures)
        self.assertEqual((chosen, method), (3, 'global_search+group'))  # posicao no grupo decide
        chosen, _, method = tex.english_counterpart(item, [self.dar([]), orig[1]], {1, 2, 3}, {2, 3}, textures)
        self.assertEqual((chosen, method), (2, 'global_search'))  # sem grupo: americano, nao japones

    def test_text_heavy_screen_falls_back_to_group_position(self):
        rng = np.random.default_rng(8)
        spanish = rng.integers(0, 256, (16, 16), dtype=np.uint8)
        english = rng.integers(0, 256, (16, 16), dtype=np.uint8)
        orig = [self.dar([(5, self.ps2(english))]), self.dar([(9, self.ps2(spanish))])]
        item = {'dar': 1, 'id': 9, 'orig': self.ps2(spanish)}
        chosen, score, method = tex.english_counterpart(item, orig, {5}, {5}, {5: english})
        self.assertEqual((chosen, method), (5, 'group_position'))
        self.assertLess(score, tex.MIN_MATCH)


if __name__ == '__main__':
    unittest.main()
