"""Gera os bancos de voz PT-BR (sound/se/sd###.xwb) a partir da ISO PS2 traduzida.

PS2: SOUND/SD.BIN tem 203 bancos (indice setor/bytes no inicio); cada banco tem
um HD Sony (IECSsreV/IECSdaeH/IECSigaV) e o BD com amostras VAG (PS-ADPCM).
O patch PT-BR regravou amostras de voz nos bancos 2, 126, 131, 133, 135, 137,
139, 141 e 143 sem mudar tamanhos nem o HD.
Xbox: sd###.xwb (WBND v1) com entradas de 20 bytes (formato, offset, bytes,
loop). O Xbox guarda a mesma contagem de amostras do PS2 (a taxa do formato e
nominal; o jogo define a frequencia), entao a troca e amostra por amostra, sem
reamostrar. O par PS2 <-> Xbox e achado pelo conteudo da faixa original.
As vozes saem em Xbox ADPCM (IMA, bloco de 36 bytes, 64 amostras tocadas),
como o runtime decodifica em src/common/dsound_runtime.inc.

Uso:
  python tools/ps2_voice_dub.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original> \
      [--xwb-dir assets/sound/se] \
      [--out-dir out/build/release/mods/PTBR-PS2/files/sound/se] \
      [--report .local/ptbr-ps2/voices.json]
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np

try:
    from ps2_movie_dub import read_iso_file, snr_db
except ModuleNotFoundError:
    from tools.ps2_movie_dub import read_iso_file, snr_db

SECTOR = 2048
BANKS = [2, 126, 131, 133, 135, 137, 139, 141, 143]
XWB_ENTRY = 0x50
XBOX_ADPCM_TAG = 1  # formato WBND v1: bit 0 tag, bits 1-3 canais, bits 4-21 taxa
BLOCK, BLOCK_SAMPLES = 36, 64

VAG_F0 = (0, 60, 115, 98, 122)
VAG_F1 = (0, 0, -52, -55, -60)
IMA_INDEX = np.array([-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8])
IMA_STEP = np.array([7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60,
                     66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371,
                     408, 449, 494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
                     2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845,
                     8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086,
                     29794, 32767])
INDEX_CANDIDATES = np.arange(0, 89, 2)


# ---------- PS2 ----------

def sd_index(sd: bytes) -> list[tuple[int, int]]:
    """Indice do SD.BIN: u32 versao, u32 quantidade, depois (setor, bytes) em +0x10."""
    count = struct.unpack_from('<I', sd, 4)[0]
    return [struct.unpack_from('<II', sd, 0x10 + 8 * i) for i in range(count)]


def hd_samples(bank: bytes) -> list[dict]:
    """Amostras VAG de um banco: posicao no banco, fim e taxa (chunk Vagi do HD)."""
    hd = bank.find(b'IECSsreV')
    head = bank.find(b'IECSdaeH')
    vagi = bank.find(b'IECSigaV')
    if min(hd, head, vagi) < 0:
        raise ValueError('Banco sem HD Sony')
    hd_size, bd_size = struct.unpack_from('<II', bank, head + 12)
    bd = hd + (hd_size + SECTOR - 1) // SECTOR * SECTOR
    last = struct.unpack_from('<I', bank, vagi + 12)[0]
    entries = []
    for rel in struct.unpack_from(f'<{last + 1}I', bank, vagi + 16):
        offset, rate, loop = struct.unpack_from('<IHB', bank, vagi + rel)
        entries.append({'start': bd + offset, 'rate': rate, 'loop': loop})
    for i, entry in enumerate(entries):
        entry['end'] = entries[i + 1]['start'] if i + 1 < len(entries) else bd + bd_size
    return entries


def decode_vag(data: bytes) -> np.ndarray:
    """PS-ADPCM: quadros de 16 bytes (filtro/shift, flags, 28 nibbles); para no flag de fim."""
    out = []
    hist1 = hist2 = 0
    for pos in range(0, len(data) - 15, 16):
        shift, predictor = data[pos] & 0xF, min(data[pos] >> 4, 4)
        flags = data[pos + 1]
        if flags == 7:
            break
        f0, f1 = VAG_F0[predictor], VAG_F1[predictor]
        for i in range(28):
            nib = (data[pos + 2 + i // 2] >> (4 * (i & 1))) & 0xF
            value = ((nib << 28) - ((nib & 8) << 29)) >> (16 + shift)
            sample = value + ((hist1 * f0 + hist2 * f1 + 32) >> 6)
            sample = max(-32768, min(32767, sample))
            out.append(sample)
            hist2, hist1 = hist1, sample
        if flags & 1:
            break
    return np.array(out, np.int16)


def vag_pcm(data: bytes) -> np.ndarray:
    """Amostra como o Xbox guarda: sem o quadro inicial zerado do VAG (28 amostras),
    medido por correlacao cruzada (atraso -28, pico 0,997 no sd126)."""
    pcm = decode_vag(data)
    return pcm[28:] if not any(data[:16]) else pcm


# ---------- Xbox ----------

def xwb_entries(xwb: bytes) -> tuple[int, list[list[int]]]:
    if xwb[:4] != b'WBND' or struct.unpack_from('<I', xwb, 4)[0] != 1:
        raise ValueError('XWB v1 esperado')
    count = struct.unpack_from('<I', xwb, 12)[0]
    entries = [list(struct.unpack_from('<5I', xwb, XWB_ENTRY + 20 * i)) for i in range(count)]
    data_start = XWB_ENTRY + 20 * count
    return data_start, entries


def ima_decode_blocks(blocks: np.ndarray) -> np.ndarray:
    """(B, 36) uint8 mono -> (B, 64) int16, como DecodeBlock do runtime."""
    blocks = blocks.astype(np.int64)
    pred = ((blocks[:, 0] | blocks[:, 1] << 8) + 0x8000 & 0xFFFF) - 0x8000
    index = np.minimum(blocks[:, 2], 88)
    out = np.empty((len(blocks), BLOCK_SAMPLES), np.int64)
    out[:, 0] = pred
    for i in range(BLOCK_SAMPLES - 1):
        nib = (blocks[:, 4 + i // 2] >> (4 * (i & 1))) & 0xF
        pred, index = ima_step(pred, index, nib)
        out[:, i + 1] = pred
    return out.astype(np.int16)


def ima_step(pred, index, nib):
    step = IMA_STEP[index]
    diff = step >> 3
    diff = diff + np.where(nib & 1, step >> 2, 0) + np.where(nib & 2, step >> 1, 0) + np.where(nib & 4, step, 0)
    pred = np.clip(np.where(nib & 8, pred - diff, pred + diff), -32768, 32767)
    return pred, np.clip(index + IMA_INDEX[nib], 0, 88)


def ima_encode(pcm: np.ndarray) -> bytes:
    """Mono int16 -> blocos Xbox ADPCM; cada bloco testa varios indices iniciais."""
    count = -(-len(pcm) // BLOCK_SAMPLES)
    padded = np.zeros(count * BLOCK_SAMPLES, np.int64)
    padded[:len(pcm)] = pcm
    blocks = padded.reshape(count, BLOCK_SAMPLES)
    k = len(INDEX_CANDIDATES)
    pred = np.repeat(blocks[:, :1], k, axis=1)
    index = np.broadcast_to(INDEX_CANDIDATES, (count, k)).copy()
    error = np.zeros((count, k), np.int64)
    nibs = np.zeros((count, k, BLOCK_SAMPLES), np.int64)
    for i in range(BLOCK_SAMPLES - 1):
        target = blocks[:, i + 1:i + 2]
        delta = target - pred
        nib = np.where(delta < 0, 8, 0)
        delta = np.abs(delta)
        step = IMA_STEP[index]
        for bit, part in ((4, step), (2, step >> 1), (1, step >> 2)):
            hit = delta >= part
            nib = nib | np.where(hit, bit, 0)
            delta = delta - np.where(hit, part, 0)
        pred, index = ima_step(pred, index, nib)
        error += (pred - target) ** 2
        nibs[:, :, i] = nib
    best = np.argmin(error, axis=1)
    rows = np.arange(count)
    chosen = nibs[rows, best]
    out = np.zeros((count, BLOCK), np.uint8)
    first = blocks[:, 0] & 0xFFFF
    out[:, 0], out[:, 1] = first & 0xFF, first >> 8
    out[:, 2] = INDEX_CANDIDATES[best]
    out[:, 4:] = (chosen[:, 0::2] | chosen[:, 1::2] << 4).astype(np.uint8)
    return out.tobytes()


def xwb_entry_pcm(xwb: bytes, data_start: int, entry: list[int]) -> np.ndarray:
    fmt, offset, size = entry[:3]
    data = xwb[data_start + offset:data_start + offset + size]
    if fmt & 1 == XBOX_ADPCM_TAG and (fmt >> 1) & 7 == 1:
        return ima_decode_blocks(np.frombuffer(data, np.uint8)[:size // BLOCK * BLOCK].reshape(-1, BLOCK)).reshape(-1)
    if fmt & 1 == 0 and (fmt >> 1) & 7 == 1:
        return np.frombuffer(data[:size // 2 * 2], '<i2')
    raise ValueError(f'Formato XWB nao suportado: {fmt:#x}')


def rebuild_xwb(xwb: bytes, replacements: dict[int, bytes]) -> bytes:
    """Troca dados de entradas Xbox ADPCM mono e recalcula offsets/tamanhos."""
    data_start, entries = xwb_entries(xwb)
    body = bytearray()
    table = bytearray()
    for i, entry in enumerate(entries):
        fmt, offset, size, loop_start, loop_length = entry
        data = replacements.get(i, xwb[data_start + offset:data_start + offset + size])
        if i in replacements:
            if fmt & 1 != XBOX_ADPCM_TAG or (fmt >> 1) & 7 != 1 or loop_length:
                raise ValueError(f'Entrada {i}: so voz Xbox ADPCM sem loop e substituida')
            size = len(data)
        table += struct.pack('<5I', fmt, len(body), size, loop_start, loop_length)
        body += data
        # Mantem o alinhamento das entradas originais (blocos de 36 bytes ja alinham).
    return bytes(xwb[:XWB_ENTRY]) + bytes(table) + bytes(body)


# ---------- pareamento ----------

def similarity(a: np.ndarray, b: np.ndarray) -> float:
    size = min(len(a), len(b))
    if size < 64:
        return 0.0
    x, y = a[:size].astype(np.float64), b[:size].astype(np.float64)
    den = np.linalg.norm(x) * np.linalg.norm(y)
    return float(x @ y / den) if den else 0.0


def match_samples(ps2: list[np.ndarray], xbox: list[np.ndarray]) -> dict[int, tuple[int, float]]:
    """PS2 i -> (Xbox j, similaridade), procurando perto do mesmo indice."""
    result = {}
    for i, pcm in enumerate(ps2):
        best = (-1, 0.0)
        for j in range(max(0, i - 3), min(len(xbox), i + 4)):
            if abs(len(xbox[j]) - len(pcm)) > max(256, len(pcm) // 20):
                continue
            score = similarity(pcm, xbox[j])
            if score > best[1]:
                best = (j, score)
        result[i] = best
    return result


def build_bank(number: int, orig_bank: bytes, ptbr_bank: bytes, xwb: bytes) -> tuple[bytes, dict]:
    samples = hd_samples(orig_bank)
    if samples != hd_samples(ptbr_bank):
        raise ValueError(f'Banco {number}: HD PT-BR difere do original')
    data_start, entries = xwb_entries(xwb)
    xbox = [xwb_entry_pcm(xwb, data_start, e) for e in entries]
    ps2 = [vag_pcm(orig_bank[s['start']:s['end']]) for s in samples]
    pairs = match_samples(ps2, xbox)
    replacements, report = {}, {'replaced': {}, 'unmatched_changed': [], 'unchanged': 0}
    for i, sample in enumerate(samples):
        if orig_bank[sample['start']:sample['end']] == ptbr_bank[sample['start']:sample['end']]:
            report['unchanged'] += 1
            continue
        j, score = pairs[i]
        if j < 0 or score < 0.9:
            report['unmatched_changed'].append({'ps2': i, 'best': j, 'score': round(score, 3)})
            continue
        if j in replacements:
            raise ValueError(f'Banco {number}: entrada Xbox {j} pareada duas vezes')
        pcm = vag_pcm(ptbr_bank[sample['start']:sample['end']])
        encoded = ima_encode(pcm)
        check = ima_decode_blocks(np.frombuffer(encoded, np.uint8).reshape(-1, BLOCK)).reshape(-1)[:len(pcm)]
        replacements[j] = encoded
        report['replaced'][i] = {'xbox': j, 'match': round(score, 3), 'samples': len(pcm),
                                 'xbox_samples': len(xbox[j]), 'snr_db': round(snr_db(pcm, check), 1)}
    return rebuild_xwb(xwb, replacements), report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--ptbr-iso', type=Path, required=True)
    parser.add_argument('--orig-iso', type=Path, required=True)
    parser.add_argument('--xwb-dir', type=Path, default=Path('assets/sound/se'))
    parser.add_argument('--out-dir', type=Path, default=Path('out/build/release/mods/PTBR-PS2/files/sound/se'))
    parser.add_argument('--report', type=Path, default=Path('.local/ptbr-ps2/voices.json'))
    args = parser.parse_args()

    orig_sd = read_iso_file(args.orig_iso, 'SOUND/SD.BIN')
    ptbr_sd = read_iso_file(args.ptbr_iso, 'SOUND/SD.BIN')
    index = sd_index(orig_sd)
    if index != sd_index(ptbr_sd):
        raise ValueError('Indice do SD.BIN PT-BR difere do original')
    args.out_dir.mkdir(parents=True, exist_ok=True)
    report = {}
    for number in BANKS:
        sector, size = index[number]
        span = slice(sector * SECTOR, sector * SECTOR + size)
        name = f'sd{number:03d}.xwb'
        xwb = (args.xwb_dir / name).read_bytes()
        rebuilt, info = build_bank(number, orig_sd[span], ptbr_sd[span], xwb)
        (args.out_dir / name).write_bytes(rebuilt)
        info.update(xwb_sha256=hashlib.sha256(xwb).hexdigest(), out_sha256=hashlib.sha256(rebuilt).hexdigest())
        report[name] = info
        snrs = [r['snr_db'] for r in info['replaced'].values()]
        print(f'{name}: trocadas {len(info["replaced"])}, sem par {len(info["unmatched_changed"])}, '
              f'iguais {info["unchanged"]}, SNR min {min(snrs, default=0):.1f} dB', flush=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=1) + '\n', encoding='utf-8')
    # Vozes PS2 sem par ficam no relatorio: o Xbox nao tem a mesma fala (sd002 #4, sd131 #10).
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
