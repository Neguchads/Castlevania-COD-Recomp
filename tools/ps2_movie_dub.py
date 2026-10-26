"""Gera o audio PT-BR das cenas (movie_sound_E.cv2) a partir da ISO PS2 traduzida.

Xbox: movie_sound_E.cv2 guarda uma faixa XMD por filme (5.1, 44,1 kHz, ADPCM
Konami). PS2: cada PSS tem PCM16 estereo 48 kHz; o patch PT-BR trocou a faixa
inglesa mantendo o mesmo numero de amostras. O atraso entre PS2 e Xbox e medido
por filme na faixa inglesa original e reaplicado na faixa PT-BR, que vai para os
canais frontais (centro, LFE e surround ficam em silencio). O CV2 original e
usado como molde: tamanhos, setores e cabecalhos XMD ficam identicos.

Codec XMD v2, conferido com o vgmstream (xmd_decoder.c, "byte-accurate" contra
o xbe deste jogo): quadro de 21 bytes por canal, canais intercalados quadro a
quadro; hist2 s16, hist1 s16, escala u16 e 15 bytes de nibbles (baixo primeiro);
amostra = (nibble*escala*16384 + hist1*0x7298 - hist2*0x3350) >> 14.

Uso:
  python tools/ps2_movie_dub.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original> \
      [--cv2 assets/movie/movie_sound_E.cv2] \
      [--out out/build/release/mods/PTBR-PS2/files/movie/movie_sound_E.cv2] \
      [--report .local/ptbr-ps2/movies.json]
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np

SECTOR = 2048
FRAME = 0x15
SAMPLES = 32
HEADER = 0x11
COEF1, COEF2 = 0x7298, 0x3350
# Escala maxima: nibble*escala*16384 + historicos cabe em int32 no decodificador.
MAX_SCALE = 4096
SCALES = np.unique(np.round(np.geomspace(1, MAX_SCALE, 120))).astype(np.int64)
XBOX_RATE, PS2_RATE = 44100, 48000
LOW_RATE = 8000
# Atraso tipico Xbox <- PS2 (mediana medida nos 23 filmes, faixa inglesa);
# usado quando a faixa tem poucas janelas confiaveis (ST05_00, ST08_00).
DEFAULT_LAG = -0.00525
MIN_WINDOWS = 5

# Ordem das faixas do CV2 Xbox -> PSS do PS2 (23 filmes alterados pelo patch).
MOVIES = ['EX/ST00_00S', 'ST01_00', 'ST02_00', 'ST03_00', 'ST04_00', 'ST05_00', 'ST05T01',
          'ST06_00', 'ST07_00', 'ST08_00', 'ST08_01', 'ST09_00', 'ST09_01', 'ST10_00',
          'ST11_00', 'ST11_01', 'ST12_00', 'ST13_00', 'ST14_00', 'ST14_01', 'ST15_00',
          'ST15_01', 'ST16_00']


# ---------- codec XMD ----------

def wrap16(values):
    return ((values + 0x8000) & 0xFFFF) - 0x8000


def predict(hist1, hist2):
    return (hist1 * COEF1 - hist2 * COEF2) >> 14


def decode_frames(frames: np.ndarray) -> np.ndarray:
    """frames: (F, 21) uint8 de um canal -> (F, 32) int16."""
    frames = frames.astype(np.int64)
    hist2 = wrap16(frames[:, 0] | frames[:, 1] << 8)
    hist1 = wrap16(frames[:, 2] | frames[:, 3] << 8)
    scale = frames[:, 4] | frames[:, 5] << 8
    out = np.empty((len(frames), SAMPLES), np.int64)
    out[:, 0], out[:, 1] = hist2, hist1
    for i in range(30):
        byte = frames[:, 6 + i // 2]
        nib = (byte >> 4) if i & 1 else (byte & 0xF)
        nib = np.where(nib >= 8, nib - 16, nib)
        sample = wrap16(nib * scale + predict(hist1, hist2))
        out[:, 2 + i] = sample
        hist2, hist1 = hist1, sample
    return out.astype(np.int16)


def encode_frames(pcm: np.ndarray, chunk: int = 8192) -> np.ndarray:
    """pcm: (F*32,) int16 de um canal -> (F, 21) uint8, escolhendo a escala de menor erro."""
    blocks = pcm.astype(np.int64).reshape(-1, SAMPLES)
    out = np.zeros((len(blocks), FRAME), np.uint8)
    for start in range(0, len(blocks), chunk):
        part = blocks[start:start + chunk]
        silent = ~part.any(axis=1)
        work = part[~silent]
        if not len(work):
            continue
        count = len(work)
        scale = np.broadcast_to(SCALES, (count, len(SCALES)))
        hist2 = np.repeat(work[:, :1], len(SCALES), axis=1)
        hist1 = np.repeat(work[:, 1:2], len(SCALES), axis=1)
        error = np.zeros((count, len(SCALES)), np.int64)
        nibs = np.empty((count, len(SCALES), 30), np.int64)
        for i in range(30):
            pred = predict(hist1, hist2)
            target = work[:, 2 + i:3 + i]
            nib = np.clip(np.rint((target - pred) / scale), -8, 7).astype(np.int64)
            sample = wrap16(nib * scale + pred)
            error += (sample - target) ** 2
            nibs[:, :, i] = nib
            hist2, hist1 = hist1, sample
        best = np.argmin(error, axis=1)
        rows = np.arange(count)
        chosen = nibs[rows, best] & 0xF
        frames = np.empty((count, FRAME), np.uint8)
        frames[:, 0:2] = np.stack([work[:, 0] & 0xFF, (work[:, 0] >> 8) & 0xFF], axis=1)
        frames[:, 2:4] = np.stack([work[:, 1] & 0xFF, (work[:, 1] >> 8) & 0xFF], axis=1)
        chosen_scale = SCALES[best]
        frames[:, 4:6] = np.stack([chosen_scale & 0xFF, chosen_scale >> 8], axis=1)
        frames[:, 6:] = (chosen[:, 0::2] | chosen[:, 1::2] << 4).astype(np.uint8)
        target_rows = np.flatnonzero(~silent) + start
        out[target_rows] = frames
    return out


def xmd_header(xmd: bytes) -> dict:
    if xmd[:3] != b'xmd':
        raise ValueError('XMD invalido')
    rate, size = struct.unpack_from('<HI', xmd, 4)
    return {'channels': xmd[3], 'rate': rate, 'data_size': size}


def decode_xmd(xmd: bytes, channels=None) -> np.ndarray:
    head = xmd_header(xmd)
    ch = head['channels']
    frames = np.frombuffer(xmd, np.uint8, head['data_size'], HEADER).reshape(-1, ch, FRAME)
    wanted = range(ch) if channels is None else channels
    return np.stack([decode_frames(frames[:, c]).reshape(-1) for c in wanted], axis=1)


def encode_xmd_payload(pcm: np.ndarray) -> bytes:
    """pcm: (N, canais) int16 com N multiplo de 32 -> dados intercalados por quadro."""
    encoded = [encode_frames(pcm[:, c]) for c in range(pcm.shape[1])]
    return np.stack(encoded, axis=1).tobytes()


# ---------- CV2 ----------

def cv2_entries(blob: bytes) -> list[tuple[int, int]]:
    if blob[:3] != b'CV2':
        raise ValueError('CV2 invalido')
    out, pos = [], 0x10
    while pos + 8 <= len(blob):
        sector, count = struct.unpack_from('<II', blob, pos)
        if count == 0 or blob[sector * SECTOR:sector * SECTOR + 3] != b'xmd':
            break
        out.append((sector, count))
        pos += 8
    return out


# ---------- ISO e PSS ----------

def iso_file(iso: Path, path: str) -> tuple[int, int]:
    """Offset e tamanho de um arquivo (com subpastas) numa ISO9660."""
    with iso.open('rb') as f:
        f.seek(16 * SECTOR)
        pvd = f.read(SECTOR)
        if pvd[1:6] != b'CD001':
            raise ValueError('ISO9660 nao encontrado')
        lba, size = struct.unpack_from('<I', pvd, 158)[0], struct.unpack_from('<I', pvd, 166)[0]
        parts = [p for p in path.upper().split('/') if p]
        for name in parts:
            f.seek(lba * SECTOR)
            directory = f.read(size)
            pos, found = 0, None
            while pos < len(directory):
                length = directory[pos]
                if length == 0:
                    pos = (pos // SECTOR + 1) * SECTOR
                    continue
                ident = directory[pos + 33:pos + 33 + directory[pos + 32]].decode('ascii', 'replace')
                if ident.split(';')[0].upper() == name:
                    found = (struct.unpack_from('<I', directory, pos + 2)[0],
                             struct.unpack_from('<I', directory, pos + 10)[0])
                    break
                pos += length
            if not found:
                raise FileNotFoundError(path)
            lba, size = found
    return lba * SECTOR, size


def read_iso_file(iso: Path, path: str) -> bytes:
    offset, size = iso_file(iso, path)
    with iso.open('rb') as f:
        f.seek(offset)
        return f.read(size)


def pss_audio(data: bytes, track: int = 0) -> tuple[np.ndarray, dict]:
    """Faixa PCM16 de private_stream_1 (SShd/SSbd) -> (N, 2) int16 e parametros."""
    raw = bytearray()
    pos, size = 0, len(data)
    while pos + 4 <= size:
        if data[pos:pos + 3] != b'\0\0\1':
            pos += 1
            continue
        sid = data[pos + 3]
        if sid == 0xBA:
            pos += 14 + (data[pos + 13] & 7)
            continue
        if sid == 0xB9:
            break
        length = struct.unpack_from('>H', data, pos + 4)[0]
        start, end = pos + 6, pos + 6 + length
        if sid == 0xBD:
            body = data[start + 3 + data[start + 2]:end]
            # Subcabecalho: subID, 0xA0, 0x00, faixa.
            if len(body) > 4 and body[3] == track:
                raw += body[4:]
        pos = end
    if raw[:4] != b'SShd':
        raise ValueError('Faixa sem SShd')
    fmt, rate, channels, interleave = struct.unpack_from('<4I', raw, 8)
    body = raw.find(b'SSbd')
    if fmt != 1 or channels not in (1, 2) or body < 0:
        raise ValueError(f'Formato PSS nao suportado: fmt={fmt} canais={channels}')
    length = struct.unpack_from('<I', raw, body + 4)[0]
    pcm = np.frombuffer(bytes(raw[body + 8:body + 8 + length]), '<i2')
    # Intercalacao 0 (ST11_01 PT-BR, mono): amostras contiguas.
    per_block = interleave // 2 if interleave else len(pcm) // channels
    pcm = pcm[:len(pcm) // (channels * per_block) * channels * per_block]
    blocks = pcm.reshape(-1, channels, per_block).transpose(0, 2, 1).reshape(-1, channels)
    # ST11_01 PT-BR e mono: vai igual para os dois lados.
    stereo = np.repeat(blocks, 2, axis=1) if channels == 1 else blocks
    return stereo, {'rate': rate, 'channels': channels, 'interleave': interleave, 'samples': len(stereo)}


# ---------- sinal ----------

def resample(signal: np.ndarray, src: int, dst: int) -> np.ndarray:
    """Reamostragem por FFT (sinal inteiro, canais em colunas)."""
    count = int(round(len(signal) * dst / src))
    spectrum = np.fft.rfft(signal.astype(np.float64), axis=0)
    bins = count // 2 + 1
    if bins <= len(spectrum):
        spectrum = spectrum[:bins]
    else:
        spectrum = np.concatenate([spectrum, np.zeros((bins - len(spectrum),) + spectrum.shape[1:])])
    return np.fft.irfft(spectrum, count, axis=0) * (count / len(signal))


def measure_lag(reference: np.ndarray, ref_rate: int, signal: np.ndarray, rate: int,
                limit_s: float = 0.25) -> tuple[float, float]:
    """Atraso (s) a somar no sinal para alinhar com a referencia, e correlacao."""
    a = resample(reference, ref_rate, LOW_RATE)
    b = resample(signal, rate, LOW_RATE)
    size = 1 << int(np.ceil(np.log2(len(a) + len(b))))
    corr = np.fft.irfft(np.fft.rfft(a - a.mean(), size) * np.conj(np.fft.rfft(b - b.mean(), size)), size)
    limit = int(limit_s * LOW_RATE)
    lags = np.concatenate([np.arange(0, limit + 1), np.arange(-limit, 0)])
    window = np.concatenate([corr[:limit + 1], corr[-limit:]])
    best = int(np.argmax(window))
    norm = window[best] / (np.linalg.norm(a - a.mean()) * np.linalg.norm(b - b.mean()) + 1e-9)
    return lags[best] / LOW_RATE, float(norm)


def windowed_lag(reference: np.ndarray, ref_rate: int, signal: np.ndarray, rate: int,
                 window_s: float = 4.0, min_corr: float = 0.3) -> tuple[float | None, int]:
    """Mediana dos atrasos em janelas confiaveis. A correlacao global erra quando
    a mixagem 5.1 do Xbox difere do estereo do PS2 em trechos longos."""
    lags = []
    duration = min(len(reference) / ref_rate, len(signal) / rate)
    for start in np.arange(0, duration - window_s, window_s):
        a = reference[int(start * ref_rate):int((start + window_s) * ref_rate)]
        b = signal[int(start * rate):int((start + window_s) * rate)]
        if np.std(a) < 50 or np.std(b) < 50:
            continue
        lag, corr = measure_lag(a, ref_rate, b, rate, 0.1)
        if corr > min_corr:
            lags.append(lag)
    if len(lags) < MIN_WINDOWS:
        return None, len(lags)
    return float(np.median(lags)), len(lags)


def lag_curve(reference: np.ndarray, signal: np.ndarray, rate: int, window_s: float = 4.0,
              hop_s: float = 2.0, limit_s: float = 0.3, min_corr: float = 0.3,
              outlier_s: float = 0.015) -> tuple[np.ndarray, np.ndarray]:
    """Atraso do sinal ao longo do tempo (centro da janela, atraso), so em janelas
    confiaveis. Algumas faixas PT-BR derivam devagar em relacao a original (ST04_00
    vai de 1 ms a 63 ms em 84 s), entao um atraso unico nao basta."""
    times, lags = [], []
    duration = min(len(reference), len(signal)) / rate
    for start in np.arange(0, max(0.0, duration - window_s) + 1e-9, hop_s):
        a = reference[int(start * rate):int((start + window_s) * rate)]
        b = signal[int(start * rate):int((start + window_s) * rate)]
        if len(a) < rate or np.std(a) < 50 or np.std(b) < 50:
            continue
        lag, corr = measure_lag(a, rate, b, rate, limit_s)
        if corr > min_corr:
            times.append(start + window_s / 2)
            lags.append(lag)
    times, lags = np.array(times), np.array(lags)
    # Descarta pontos que fogem da vizinhanca (+-10 s): correlacao alta por acaso.
    keep = np.array([abs(lag - np.median(lags[np.abs(times - t) <= 10])) <= outlier_s
                     for t, lag in zip(times, lags)], bool)
    return times[keep] if len(times) else times, lags[keep] if len(lags) else lags


def warp(signal: np.ndarray, rate: int, times: np.ndarray, lags: np.ndarray) -> np.ndarray:
    """Desloca cada instante pelo atraso interpolado (constante fora dos pontos)."""
    if not len(times):
        return signal.astype(np.float64)
    t = np.arange(len(signal)) / rate
    source = (t - np.interp(t, times, lags)) * rate
    positions = np.arange(len(signal))
    columns = [np.interp(source, positions, signal[:, c].astype(np.float64), left=0.0, right=0.0)
               for c in range(signal.shape[1])]
    return np.stack(columns, axis=1)


def place(signal: np.ndarray, lag_s: float, rate: int, length: int) -> np.ndarray:
    shift = int(round(lag_s * rate))
    out = np.zeros((length,) + signal.shape[1:], signal.dtype)
    src = signal[max(0, -shift):]
    dst = max(0, shift)
    count = max(0, min(len(src), length - dst))
    out[dst:dst + count] = src[:count]
    return out


def to_int16(signal: np.ndarray) -> np.ndarray:
    return np.clip(np.rint(signal), -32768, 32767).astype(np.int16)


def snr_db(reference: np.ndarray, test: np.ndarray) -> float:
    noise = np.sum((reference.astype(np.float64) - test) ** 2)
    power = np.sum(reference.astype(np.float64) ** 2)
    return float(10 * np.log10(power / max(noise, 1.0)))


def build_movie(xmd: bytes, ptbr: np.ndarray, english_ps2: np.ndarray) -> tuple[bytes, dict]:
    head = xmd_header(xmd)
    if head['channels'] != 6 or head['rate'] != XBOX_RATE:
        raise ValueError(f'XMD inesperado: {head}')
    length = head['data_size'] // FRAME // head['channels'] * SAMPLES
    xbox = decode_xmd(xmd, channels=(0, 1, 2)).astype(np.float64).sum(axis=1)
    english = english_ps2.astype(np.float64).mean(axis=1)
    # PS2 PT-BR -> linha do tempo do PS2 ingles (mesma trilha de fundo, com deriva),
    # depois atraso fixo Xbox <- PS2 ingles.
    xbox_lag, xbox_windows = windowed_lag(xbox, XBOX_RATE, english, PS2_RATE)
    times, lags = lag_curve(english, ptbr.astype(np.float64).mean(axis=1), PS2_RATE)
    aligned = warp(ptbr, PS2_RATE, times, lags)
    lag = DEFAULT_LAG if xbox_lag is None else xbox_lag
    front = to_int16(place(resample(aligned, PS2_RATE, XBOX_RATE), lag, XBOX_RATE, length))
    pcm = np.zeros((length, head['channels']), np.int16)
    pcm[:, 0:2] = front
    payload = encode_xmd_payload(pcm)
    if len(payload) != head['data_size']:
        raise ValueError('Tamanho XMD divergente')
    check = decode_xmd(xmd[:HEADER] + payload, channels=(0, 1))
    return payload, {'lag_ms': round(lag * 1000, 2),
                     'xbox_lag_ms': None if xbox_lag is None else round(xbox_lag * 1000, 2),
                     'xbox_windows': xbox_windows,
                     'ptbr_points': len(times),
                     'ptbr_lag_ms_range': [round(float(lags.min()) * 1000, 2), round(float(lags.max()) * 1000, 2)]
                     if len(lags) else None,
                     'xbox_s': round(length / XBOX_RATE, 3), 'ps2_s': round(len(ptbr) / PS2_RATE, 3),
                     'snr_db': round(snr_db(front, check), 1)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--ptbr-iso', type=Path, required=True)
    parser.add_argument('--orig-iso', type=Path, required=True)
    parser.add_argument('--cv2', type=Path, default=Path('assets/movie/movie_sound_E.cv2'))
    parser.add_argument('--out', type=Path,
                        default=Path('out/build/release/mods/PTBR-PS2/files/movie/movie_sound_E.cv2'))
    parser.add_argument('--report', type=Path, default=Path('.local/ptbr-ps2/movies.json'))
    parser.add_argument('--only', type=int, nargs='*', help='indices das faixas (teste)')
    args = parser.parse_args()

    template = args.cv2.read_bytes()
    entries = cv2_entries(template)
    if len(entries) < len(MOVIES):
        raise ValueError(f'CV2 com {len(entries)} faixas; esperado >= {len(MOVIES)}')
    output = bytearray(template)
    report = {'cv2_sha256': hashlib.sha256(template).hexdigest(), 'movies': {}}
    for index, name in enumerate(MOVIES):
        if args.only and index not in args.only:
            continue
        path = f'PSS/{name}.PSS'
        ptbr, ptbr_info = pss_audio(read_iso_file(args.ptbr_iso, path))
        english, english_info = pss_audio(read_iso_file(args.orig_iso, path))
        sector, count = entries[index]
        start = sector * SECTOR
        xmd = template[start:start + count * SECTOR]
        payload, info = build_movie(xmd, ptbr, english)
        output[start + HEADER:start + HEADER + len(payload)] = payload
        info.update(pss=path, ptbr_header_rate=ptbr_info['rate'], ptbr_channels=ptbr_info['channels'],
                    ptbr_samples=ptbr_info['samples'], english_samples=english_info['samples'])
        report['movies'][index] = info
        print(f'{index:2d} {name:12s} atraso {info["lag_ms"]:7.2f} ms (xbox {info["xbox_lag_ms"]}/'
              f'{info["xbox_windows"]}, pt {info["ptbr_lag_ms_range"]}/{info["ptbr_points"]}) '
              f'SNR {info["snr_db"]:.1f} dB  {info["ps2_s"]:.2f}s -> {info["xbox_s"]:.2f}s', flush=True)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(bytes(output))
    report['out_sha256'] = hashlib.sha256(output).hexdigest()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
