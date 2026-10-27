import struct
import tempfile
import unittest
from pathlib import Path

import numpy as np

try:
    import ps2_movie_dub as dub
except ModuleNotFoundError:
    from tools import ps2_movie_dub as dub


def frame(hist2, hist1, scale, nibbles):
    packed = bytes((nibbles[i] & 0xF) | (nibbles[i + 1] & 0xF) << 4 for i in range(0, 30, 2))
    return struct.pack('<hhH', hist2, hist1, scale) + packed


def reference_decode(data):
    """Decodificador escalar, escrito direto da formula do xbe (vgmstream)."""
    hist2, hist1, scale = struct.unpack_from('<hhH', data, 0)
    out = [hist2, hist1]
    for i in range(30):
        nib = (data[6 + i // 2] >> 4) if i & 1 else data[6 + i // 2] & 0xF
        nib -= 16 if nib >= 8 else 0
        sample = (nib * (scale << 14) + hist1 * 0x7298 - hist2 * 0x3350) >> 14
        sample = ((sample + 0x8000) & 0xFFFF) - 0x8000
        out.append(sample)
        hist2, hist1 = hist1, sample
    return out


class CodecTests(unittest.TestCase):
    def test_decoder_matches_scalar_formula_including_wrap(self):
        rng = np.random.default_rng(1)
        cases = [frame(0, 0, 0, [0] * 30), frame(-2, -2, 1, [1, -1] * 15),
                 frame(32000, 32767, 4096, [7] * 30), frame(-32768, 100, 65535, [-8] * 30)]
        cases += [bytes(rng.integers(0, 256, 21, dtype=np.uint8)) for _ in range(50)]
        frames = np.frombuffer(b''.join(cases), np.uint8).reshape(-1, 21)
        decoded = dub.decode_frames(frames)
        for row, data in zip(decoded, cases):
            self.assertEqual(row.tolist(), reference_decode(data))

    def test_low_nibble_comes_first(self):
        data = frame(0, 0, 10, [1, 0] + [0] * 28)
        self.assertEqual(dub.decode_frames(np.frombuffer(data, np.uint8).reshape(1, 21))[0, 2], 10)

    def test_encoder_roundtrip_quality_and_silence(self):
        t = np.arange(32 * 400) / 44100
        tone = (8000 * np.sin(2 * np.pi * 440 * t) + 3000 * np.sin(2 * np.pi * 3100 * t)).astype(np.int16)
        tone[:64] = 0
        encoded = dub.encode_frames(tone)
        self.assertEqual(encoded.shape, (400, 21))
        self.assertFalse(encoded[:2].any())  # quadros silenciosos ficam zerados
        decoded = dub.decode_frames(encoded).reshape(-1)
        self.assertGreater(dub.snr_db(tone, decoded), 30)
        self.assertTrue((dub.SCALES <= dub.MAX_SCALE).all())

    def test_xmd_payload_interleaves_channels_frame_by_frame(self):
        pcm = np.zeros((64, 6), np.int16)
        pcm[:, 1] = 500
        payload = dub.encode_xmd_payload(pcm)
        self.assertEqual(len(payload), 2 * 6 * 21)
        header = b'xmd\x06' + struct.pack('<HI', 44100, len(payload)) + bytes(7)
        decoded = dub.decode_xmd(header + payload)
        self.assertEqual(decoded.shape, (64, 6))
        self.assertTrue((decoded[:, 1] == 500).all())
        self.assertFalse(decoded[:, [0, 2, 3, 4, 5]].any())
        self.assertEqual(payload[21:23], struct.pack('<h', 500))  # 2o quadro = canal 1


class ContainerTests(unittest.TestCase):
    def test_cv2_entries_stop_at_table_end(self):
        blob = bytearray(4 * dub.SECTOR)
        blob[:4] = b'CV2\x17'
        struct.pack_into('<IIII', blob, 0x10, 1, 1, 2, 2)
        blob[dub.SECTOR:dub.SECTOR + 3] = b'xmd'
        blob[2 * dub.SECTOR:2 * dub.SECTOR + 3] = b'xmd'
        self.assertEqual(dub.cv2_entries(bytes(blob)), [(1, 1), (2, 2)])
        with self.assertRaises(ValueError):
            dub.cv2_entries(b'XXXX' + bytes(16))

    def test_iso_lookup_follows_subdirectories(self):
        image = bytearray(24 * dub.SECTOR)
        pvd = 16 * dub.SECTOR
        image[pvd:pvd + 6] = b'\x01CD001'

        def record(name, lba, size):
            ident = name.encode()
            rec = bytearray(33 + len(ident) + (len(ident) + 1) % 2)
            rec[0] = len(rec)
            struct.pack_into('<I', rec, 2, lba)
            struct.pack_into('<I', rec, 10, size)
            rec[32] = len(ident)
            rec[33:33 + len(ident)] = ident
            return rec

        image[pvd + 156:pvd + 190] = record('\0', 18, dub.SECTOR)[:34]
        root = record('PSS', 19, dub.SECTOR)
        image[18 * dub.SECTOR:18 * dub.SECTOR + len(root)] = root
        sub = record('EX', 20, dub.SECTOR)
        image[19 * dub.SECTOR:19 * dub.SECTOR + len(sub)] = sub
        leaf = record('ST00_00S.PSS;1', 21, 5)
        image[20 * dub.SECTOR:20 * dub.SECTOR + len(leaf)] = leaf
        image[21 * dub.SECTOR:21 * dub.SECTOR + 5] = b'hello'
        with tempfile.TemporaryDirectory() as tmp:
            iso = Path(tmp) / 't.iso'
            iso.write_bytes(image)
            self.assertEqual(dub.read_iso_file(iso, 'PSS/EX/ST00_00S.PSS'), b'hello')
            with self.assertRaises(FileNotFoundError):
                dub.read_iso_file(iso, 'PSS/ST99.PSS')

    def test_pss_audio_picks_track_and_deinterleaves(self):
        left, right = np.arange(512, dtype='<i2'), -np.arange(512, dtype='<i2')
        body = b''.join(left[i:i + 256].tobytes() + right[i:i + 256].tobytes() for i in (0, 256))
        stream = b'SShd' + struct.pack('<5I', 24, 1, 48000, 2, 512) + bytes(4) + b'SSbd' + struct.pack('<I', len(body)) + body

        def packets(track, payload):
            out = b''
            for pos in range(0, len(payload), 1500):
                chunk = bytes([0xFF, 0xA0, 0, track]) + payload[pos:pos + 1500]
                pes = b'\x81\x80\x00' + chunk
                out += b'\0\0\1\xBD' + struct.pack('>H', len(pes)) + pes
            return out

        pack = b'\0\0\1\xBA' + bytes(9) + b'\xF8'
        # Faixa 1 (japones no original) chega antes e nao pode contaminar a faixa 0.
        data = pack + packets(1, bytes(len(stream))) + packets(0, stream) + b'\0\0\1\xB9'
        stereo, info = dub.pss_audio(data, track=0)
        self.assertEqual(info['rate'], 48000)
        self.assertEqual(stereo[:, 0].tolist(), left.tolist())
        self.assertEqual(stereo[:, 1].tolist(), right.tolist())
        # Faixa mono (ST11_01 PT-BR): blocos contiguos, copiados para os dois lados.
        mono = b'SShd' + struct.pack('<5I', 24, 1, 47750, 1, 512) + bytes(4) + b'SSbd' + \
            struct.pack('<I', len(left.tobytes()) * 2) + left.tobytes() + right.tobytes()
        stereo, info = dub.pss_audio(pack + packets(0, mono) + b'\0\0\1\xB9')
        self.assertEqual(info['channels'], 1)
        self.assertEqual(stereo.shape, (1024, 2))
        self.assertEqual(stereo[:512, 1].tolist(), left.tolist())
        self.assertTrue((stereo[:, 0] == stereo[:, 1]).all())


class DriftTests(unittest.TestCase):
    def test_lag_curve_and_warp_follow_slow_drift(self):
        rate = 8000
        rng = np.random.default_rng(5)
        # Ruido filtrado (graves, como voz e musica): ruido branco perde a correlacao
        # com os 4 ms de deriva dentro de uma janela.
        reference = np.convolve(rng.normal(0, 3000, rate * 60), np.ones(32) / 32 * 8, 'same')
        # Copia que atrasa 1 ms por segundo (60 ms no fim), como ST04_00 PT-BR.
        t = np.arange(len(reference)) / rate
        drifted = np.interp(t - 0.001 * t, t, reference)
        times, lags = dub.lag_curve(reference, drifted, rate)
        self.assertGreater(len(times), 20)
        self.assertLess(np.max(np.abs(lags - (-0.001 * times))), 0.002)
        fixed = dub.warp(drifted[:, None], rate, times, lags)[:, 0]
        tail = slice(rate * 50, rate * 55)
        self.assertGreater(np.corrcoef(fixed[tail], reference[tail])[0, 1], 0.9)
        self.assertLess(np.corrcoef(drifted[tail], reference[tail])[0, 1], 0.2)

    def test_outlier_window_is_dropped_and_empty_curve_keeps_signal(self):
        rate = 8000
        rng = np.random.default_rng(6)
        reference = rng.normal(0, 3000, rate * 30)
        copy = reference.copy()
        copy[rate * 14:rate * 18] = np.roll(reference, 400)[rate * 14:rate * 18]  # trecho com 50 ms errado
        _, lags = dub.lag_curve(reference, copy, rate)
        self.assertGreater(len(lags), 5)
        self.assertLess(np.max(np.abs(lags)), 0.002)
        signal = np.ones((5, 2))
        self.assertTrue(np.array_equal(dub.warp(signal, rate, np.array([]), np.array([])), signal))


class SignalTests(unittest.TestCase):
    def test_resample_keeps_duration_and_tone(self):
        t = np.arange(48000) / 48000
        out = dub.resample(np.sin(2 * np.pi * 1000 * t)[:, None], 48000, 44100)
        self.assertEqual(len(out), 44100)
        expected = np.sin(2 * np.pi * 1000 * np.arange(44100) / 44100)
        self.assertLess(np.max(np.abs(out[1000:-1000, 0] - expected[1000:-1000])), 1e-3)

    def test_measure_lag_and_place(self):
        rng = np.random.default_rng(2)
        noise = rng.normal(0, 1000, 48000 * 3)
        delayed = np.concatenate([np.zeros(240), noise])[:len(noise)]  # +5 ms
        lag, corr = dub.measure_lag(delayed, 48000, noise, 48000)
        self.assertAlmostEqual(lag, 0.005, places=3)
        self.assertGreater(corr, 0.9)
        lag, windows = dub.windowed_lag(delayed, 48000, noise, 48000, window_s=0.5)
        self.assertAlmostEqual(lag, 0.005, places=3)
        self.assertGreaterEqual(windows, dub.MIN_WINDOWS)
        lag, windows = dub.windowed_lag(delayed, 48000, rng.normal(0, 1000, len(noise)), 48000, window_s=0.5)
        self.assertIsNone(lag)  # sem janelas confiaveis: chamador usa DEFAULT_LAG
        placed = dub.place(np.arange(10)[:, None], 0.002, 1000, 8)
        self.assertEqual(placed[:, 0].tolist(), [0, 0, 0, 1, 2, 3, 4, 5])
        placed = dub.place(np.arange(10)[:, None], -0.002, 1000, 12)
        self.assertEqual(placed[:, 0].tolist(), list(range(2, 10)) + [0] * 4)


if __name__ == '__main__':
    unittest.main()
