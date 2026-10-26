import struct
import unittest

import numpy as np

try:
    import ps2_voice_dub as voice
except ModuleNotFoundError:
    from tools import ps2_voice_dub as voice


def vag_frame(predictor, shift, flags, nibbles):
    packed = bytes((nibbles[i] & 0xF) | (nibbles[i + 1] & 0xF) << 4 for i in range(0, 28, 2))
    return bytes([predictor << 4 | shift, flags]) + packed


def runtime_decode(block):
    """Copia escalar de DecodeBlock (src/common/dsound_runtime.inc), canal mono."""
    pred = struct.unpack_from('<h', block, 0)[0]
    idx = min(block[2], 88)
    out = [pred]
    for i in range(63):
        n = (block[4 + i // 2] >> ((i & 1) * 4)) & 15
        step = int(voice.IMA_STEP[idx])
        diff = step >> 3
        if n & 1:
            diff += step >> 2
        if n & 2:
            diff += step >> 1
        if n & 4:
            diff += step
        pred = max(-32768, min(32767, pred - diff if n & 8 else pred + diff))
        idx = max(0, min(88, idx + int(voice.IMA_INDEX[n])))
        out.append(pred)
    return out


def xwb(entries):
    """WBND v1: cabecalho de 0x50 bytes, entradas de 20 bytes, dados em seguida."""
    head = bytearray(voice.XWB_ENTRY)
    head[:4] = b'WBND'
    struct.pack_into('<I', head, 4, 1)
    struct.pack_into('<I', head, 12, len(entries))
    table, body = bytearray(), bytearray()
    for fmt, data, loop in entries:
        table += struct.pack('<5I', fmt, len(body), len(data), 0, loop)
        body += data
    return bytes(head + table + body)


class VagTests(unittest.TestCase):
    def test_vag_filters_shift_sign_and_end_flag(self):
        data = vag_frame(0, 12, 0, [1, -1, 7, -8] + [0] * 24) + vag_frame(1, 12, 1, [0] * 28) + vag_frame(0, 0, 0, [7] * 28)
        pcm = voice.decode_vag(data)
        self.assertEqual(len(pcm), 56)  # para no flag de fim
        self.assertEqual(pcm[:4].tolist(), [1, -1, 7, -8])
        # Filtro 1: amostra = anterior * 60/64 (arredondado), sem nibble.
        self.assertEqual(pcm[28], (0 * 60 + 32) >> 6)

    def test_xbox_layout_drops_leading_silent_frame(self):
        data = bytes(16) + vag_frame(0, 12, 1, [3] * 28)
        self.assertEqual(len(voice.decode_vag(data)), 56)
        self.assertEqual(voice.vag_pcm(data).tolist(), [3] * 28)
        loud_start = vag_frame(0, 12, 1, [2] * 28)
        self.assertEqual(len(voice.vag_pcm(loud_start)), 28)


class ImaTests(unittest.TestCase):
    def test_vector_decoder_matches_runtime_block_decoder(self):
        rng = np.random.default_rng(3)
        blocks = rng.integers(0, 256, (40, 36), dtype=np.uint8)
        blocks[:, 2] = rng.integers(0, 100, 40)  # indices > 88 sao limitados
        decoded = voice.ima_decode_blocks(blocks)
        for row, block in zip(decoded, blocks):
            self.assertEqual(row.tolist(), runtime_decode(bytes(block)))

    def test_encoder_quality_and_block_padding(self):
        t = np.arange(5000) / 16000
        speechy = (6000 * np.sin(2 * np.pi * 220 * t) * np.sin(2 * np.pi * 3 * t)).astype(np.int16)
        encoded = voice.ima_encode(speechy)
        self.assertEqual(len(encoded), -(-5000 // 64) * 36)
        decoded = voice.ima_decode_blocks(np.frombuffer(encoded, np.uint8).reshape(-1, 36)).reshape(-1)
        self.assertGreater(voice.snr_db(speechy, decoded[:5000]), 20)


class XwbTests(unittest.TestCase):
    ADPCM_MONO_22K = 0x00056223
    PCM_MONO_22K = 0x80056222

    def test_rebuild_replaces_voice_and_moves_following_entries(self):
        original = xwb([(self.ADPCM_MONO_22K, b'A' * 72, 0), (self.PCM_MONO_22K, b'\1\0' * 8, 0),
                        (self.ADPCM_MONO_22K, b'C' * 36, 0)])
        rebuilt = voice.rebuild_xwb(original, {0: b'N' * 108})
        start, entries = voice.xwb_entries(rebuilt)
        self.assertEqual([e[1:3] for e in entries], [[0, 108], [108, 16], [124, 36]])
        self.assertEqual(rebuilt[start:start + 108], b'N' * 108)
        self.assertEqual(rebuilt[start + 108:start + 124], b'\1\0' * 8)
        self.assertEqual(rebuilt[:voice.XWB_ENTRY], original[:voice.XWB_ENTRY])
        self.assertEqual(voice.rebuild_xwb(original, {}), original)

    def test_rebuild_refuses_pcm_or_looped_entries(self):
        original = xwb([(self.PCM_MONO_22K, b'\0' * 4, 0), (self.ADPCM_MONO_22K, b'L' * 36, 64)])
        for index in (0, 1):
            with self.assertRaises(ValueError):
                voice.rebuild_xwb(original, {index: b'X' * 36})

    def test_format_word_decodes_pcm_and_adpcm(self):
        pcm = np.array([1, -2, 3], '<i2').tobytes()
        block = bytes([0x10, 0, 0, 0]) + bytes(32)
        bank = xwb([(self.PCM_MONO_22K, pcm, 0), (self.ADPCM_MONO_22K, block, 0)])
        start, entries = voice.xwb_entries(bank)
        self.assertEqual(voice.xwb_entry_pcm(bank, start, entries[0]).tolist(), [1, -2, 3])
        self.assertEqual(voice.xwb_entry_pcm(bank, start, entries[1])[0], 16)


class MatchTests(unittest.TestCase):
    def test_matches_by_content_near_index_and_rejects_missing_line(self):
        rng = np.random.default_rng(4)
        lines = [rng.normal(0, 3000, n).astype(np.int16) for n in (800, 900, 1000, 1100)]
        xbox = [lines[0], lines[2], lines[3]]  # linha 1 nao existe no Xbox
        pairs = voice.match_samples(lines, xbox)
        self.assertEqual(pairs[0][0], 0)
        self.assertEqual(pairs[2][0], 1)
        self.assertEqual(pairs[3][0], 2)
        self.assertLess(pairs[1][1], 0.9)


if __name__ == '__main__':
    unittest.main()
