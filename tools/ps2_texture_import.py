"""Converte as imagens PT-BR da ISO PS2 traduzida para os DARs do Xbox.

PS2 (FILEDATA.BIN): cada imagem com texto existe em grupos de DARs por idioma;
o patch PT-BR regravou as texturas do slot espanhol (IDs fora do Xbox) e tres
IDs compartilhados (titulo 14452, 14056, 10114). A versao inglesa de cada uma
e achada pelo conteudo: a textura espanhola original e comparada as texturas
de mesmo formato dos DARs anteriores do grupo cujo ID existe no Xbox.

Formatos conferidos com as mesmas texturas inglesas nos dois consoles:
  PS2: +0x10 u16 offset dos pixels, u16 offset da CLUT; +0x14 largura, altura;
       +0x19 PSM (0x13 = PSMT8, 0x14 = PSMT4). PSMT8 usa unswizzle8; PSMT4 expande
       os nibbles (baixo primeiro) e aplica o mesmo unswizzle8. CLUT RGBA com
       alpha 0..0x80 e permutacao CSM1 em PSMT8.
  Xbox: mesmo cabecalho PS2 nos primeiros 0x40 bytes; D3D em +0x40 (formato P8,
       log2 de largura/altura); indices em +0x100 na ordem Morton do SDK
       (bit x primeiro); paleta de 256 cores em seguida.
So os indices (e a paleta, quando a CLUT embutida PT-BR difere da inglesa) sao
trocados; a area de preenchimento ate a potencia de 2 fica como no Xbox.

Uso:
  python tools/ps2_texture_import.py --ptbr-iso <ISO PT-BR> --orig-iso <ISO original> \
      [--xbox-data assets/data] [--out-dir out/build/release/mods/PTBR-PS2/files/data] \
      [--report .local/ptbr-ps2/textures.json]
"""

import argparse
import hashlib
import json
import struct
from pathlib import Path

import numpy as np

try:
    from ps2_movie_dub import read_iso_file
    from ps2_ptbr_import import DAR_MAGIC, walk_dars
except ModuleNotFoundError:
    from tools.ps2_movie_dub import read_iso_file
    from tools.ps2_ptbr_import import DAR_MAGIC, walk_dars

TEXTURE = 2
PSMT8, PSMT4 = 0x13, 0x14
XBOX_D3D, XBOX_PIXELS = 0x40, 0x100
FORMAT_P8 = 0x0B
GROUP_SPAN = 6  # DARs de idioma antes do slot espanhol (4 variantes + extras)
MIN_MATCH = 0.5        # candidato de DAR americano (15707 -> 15506 = 0,53, conferido em imagem)
MIN_MATCH_OTHER = 0.8  # candidato fora dos DARs americanos (ex.: effecttex.dar)
TIE = 0.05


# ---------- DAR ----------

def dar_records(blob: bytes) -> list[tuple[int, int, int, int]]:
    """(id, tipo, inicio, tamanho) com offset relativo ao registro de cada entrada."""
    count = struct.unpack_from('<I', blob, 0x10)[0]
    out = []
    for i in range(count):
        rec = 0x20 + 16 * i
        entry_id, kind, offset, size = struct.unpack_from('<IIII', blob, rec)
        out.append((entry_id, kind, rec + offset, size))
    return out


# ---------- PS2 ----------

def unswizzle8_map(w: int, h: int) -> np.ndarray:
    """Posicao no buffer PS2 de cada pixel (y, x) em PSMT8."""
    y, x = np.mgrid[0:h, 0:w]
    block = (y & ~0xF) * w + (x & ~0xF) * 2
    swap = (((y + 2) >> 2) & 1) * 4
    py = (((y & ~3) >> 1) + (y & 1)) & 7
    column = py * w * 2 + ((x + swap) & 7) * 4
    byte = ((y >> 1) & 1) + ((x >> 2) & 2)
    return block + column + byte


def csm1(i: np.ndarray) -> np.ndarray:
    return (i & ~0x18) | ((i & 0x08) << 1) | ((i & 0x10) >> 1)


def ps2_header(entry: bytes) -> dict:
    pix, clut = struct.unpack_from('<HH', entry, 0x10)
    w, h = struct.unpack_from('<HH', entry, 0x14)
    return {'pix': pix, 'clut': clut, 'w': w, 'h': h, 'psm': entry[0x19]}


def ps2_indices(entry: bytes) -> np.ndarray:
    head = ps2_header(entry)
    w, h = head['w'], head['h']
    if head['psm'] == PSMT8:
        linear = np.frombuffer(entry, np.uint8, w * h, head['pix'])
    elif head['psm'] == PSMT4:
        packed = np.frombuffer(entry, np.uint8, w * h // 2, head['pix'])
        linear = np.stack([packed & 0xF, packed >> 4], axis=1).reshape(-1)
    else:
        raise ValueError(f'PSM {head["psm"]:#x} nao suportado')
    return linear[unswizzle8_map(w, h)]


def ps2_palette(entry: bytes) -> np.ndarray | None:
    """CLUT embutida (256 ou 16 cores) em ordem de indice; None se externa/ausente."""
    head = ps2_header(entry)
    colors = 256 if head['psm'] == PSMT8 else 16
    if head['clut'] == head['pix'] or head['clut'] + 4 * colors > len(entry):
        return None
    raw = np.frombuffer(entry, np.uint8, 4 * colors, head['clut']).reshape(colors, 4)
    if not raw.any():
        return None
    order = csm1(np.arange(colors)) if colors == 256 else np.arange(colors)
    return raw[order]


# ---------- Xbox ----------

def morton_map(w: int, h: int) -> np.ndarray:
    """Posicao Morton de cada pixel (y, x), como SwizzleMasks/Deposit do SDK."""
    mask_x = mask_y = 0
    bit = 1
    lx, ly = w.bit_length() - 1, h.bit_length() - 1
    for i in range(max(lx, ly)):
        if i < lx:
            mask_x |= bit
            bit <<= 1
        if i < ly:
            mask_y |= bit
            bit <<= 1

    def deposit(values, mask):
        out = np.zeros_like(values)
        src = 1
        dst = 0
        while mask >> dst:
            if mask >> dst & 1:
                out |= np.where(values & src, 1 << dst, 0)
                src <<= 1
            dst += 1
        return out

    y, x = np.mgrid[0:h, 0:w]
    return deposit(x, mask_x) | deposit(y, mask_y)


def xbox_layout(entry: bytes) -> dict:
    fmt = struct.unpack_from('<I', entry, XBOX_D3D + 12)[0]
    if (fmt >> 8) & 0xFF != FORMAT_P8:
        raise ValueError(f'Formato Xbox {(fmt >> 8) & 0xFF:#x} nao e P8')
    w, h = 1 << ((fmt >> 20) & 0xF), 1 << ((fmt >> 24) & 0xF)
    palette = XBOX_PIXELS + w * h
    if palette + 1024 > len(entry):
        raise ValueError('Entrada Xbox sem paleta apos os pixels')
    return {'w': w, 'h': h, 'palette': palette}


def xbox_indices(entry: bytes) -> np.ndarray:
    lay = xbox_layout(entry)
    pixels = np.frombuffer(entry, np.uint8, lay['w'] * lay['h'], XBOX_PIXELS)
    return pixels[morton_map(lay['w'], lay['h'])]


def xbox_palette_from_ps2(rgba: np.ndarray, template: np.ndarray) -> np.ndarray:
    """RGBA PS2 (alpha 0..0x80) -> BGRA Xbox (alpha 0..0xFF), preservando as cores
    que a paleta PS2 nao define (16 cores em PSMT4)."""
    out = template.copy()
    count = len(rgba)
    out[:count, 0], out[:count, 1], out[:count, 2] = rgba[:, 2], rgba[:, 1], rgba[:, 0]
    out[:count, 3] = np.minimum(255, rgba[:, 3].astype(np.int32) * 2)
    return out


def write_xbox(entry: bytes, indices: np.ndarray, palette: np.ndarray | None) -> bytes:
    lay = xbox_layout(entry)
    h, w = indices.shape
    if w > lay['w'] or h > lay['h']:
        raise ValueError('Textura PS2 maior que a Xbox')
    out = bytearray(entry)
    pixels = np.frombuffer(entry, np.uint8, lay['w'] * lay['h'], XBOX_PIXELS).copy()
    pixels[morton_map(lay['w'], lay['h'])[:h, :w]] = indices
    out[XBOX_PIXELS:XBOX_PIXELS + len(pixels)] = pixels.tobytes()
    if palette is not None:
        out[lay['palette']:lay['palette'] + 1024] = palette.astype(np.uint8).tobytes()
    return bytes(out)


def xbox_palette(entry: bytes) -> np.ndarray:
    lay = xbox_layout(entry)
    return np.frombuffer(entry, np.uint8, 1024, lay['palette']).reshape(256, 4)


# ---------- pareamento e montagem ----------

def same_shape(a: bytes, b: bytes) -> bool:
    ha, hb = ps2_header(a), ps2_header(b)
    return (ha['w'], ha['h'], ha['psm']) == (hb['w'], hb['h'], hb['psm']) and len(a) == len(b)


def agreement(a: np.ndarray, b: np.ndarray) -> float:
    return float(np.mean(a == b))


def changed_textures(orig: list[bytes], ptbr: list[bytes]) -> list[dict]:
    out = []
    for k, (a, b) in enumerate(zip(orig, ptbr)):
        if a == b:
            continue
        for (entry_id, kind, start, size), (_, _, start_b, size_b) in zip(dar_records(a), dar_records(b)):
            if kind == TEXTURE and a[start:start + size] != b[start_b:start_b + size_b]:
                out.append({'dar': k, 'id': entry_id, 'orig': a[start:start + size], 'ptbr': b[start_b:start_b + size_b]})
    return out


def english_dar(name: str) -> bool:
    """DARs Xbox da versao americana (ex.: titleue.dar). O nome sem sufixo e a
    japonesa; *ee.dar tambem traz IDs japoneses (13959, conferido em imagem)."""
    return Path(name).stem.endswith('ue')


def english_counterpart(item: dict, orig: list[bytes], xbox_ids: set[int], english_ids: set[int],
                        xbox_textures: dict[int, np.ndarray]) -> tuple[int, float, str]:
    """ID ingles do Xbox para a textura espanhola: (id, semelhanca, metodo)."""
    if item['id'] in xbox_ids:
        return item['id'], 1.0, 'shared_id'
    source = ps2_indices(item['orig'])
    h, w = source.shape
    # Mesma posicao num DAR anterior do grupo de idiomas, com ID americano.
    group = None
    position = next((i for i, rec in enumerate(dar_records(orig[item['dar']])) if rec[0] == item['id']), -1)
    for k in range(item['dar'] - 1, max(-1, item['dar'] - GROUP_SPAN - 1), -1):
        records = dar_records(orig[k])
        if position >= len(records):
            continue
        entry_id, kind, start, size = records[position]
        candidate = orig[k][start:start + size]
        if kind == TEXTURE and entry_id in english_ids and same_shape(candidate, item['orig']):
            group = (entry_id, agreement(source, ps2_indices(candidate)))
            break
    # 1) Busca em todas as texturas Xbox de mesmo tamanho. Empate tecnico favorece
    #    o DAR americano (a versao japonesa tem o mesmo fundo) e, entre cartoes de
    #    mesmo fundo (nomes de area), a posicao no grupo.
    scores = sorted(((float(np.mean(idx[:h, :w] == source)), xid) for xid, idx in xbox_textures.items()
                     if idx.shape[0] >= h and w <= idx.shape[1] < 2 * w), reverse=True)
    if scores:
        top = scores[0][0]
        english = [(s, x) for s, x in scores if x in english_ids and s >= top - 0.1]
        score, chosen = english[0] if english else scores[0]
        if score >= (MIN_MATCH if chosen in english_ids else MIN_MATCH_OTHER):
            if group and any(x == group[0] and s >= score - TIE for s, x in english):
                return group[0], next(s for s, x in english if x == group[0]), 'global_search+group'
            return chosen, score, 'global_search'
    # 2) Telas quase so de texto (dicas): a semelhanca cai com a lingua.
    if group:
        return group[0], group[1], 'group_position'
    return (scores[0][1], scores[0][0], 'global_search') if scores else (-1, 0.0, 'none')


def convert(ps2_english: bytes, ps2_ptbr: bytes, xbox_entry: bytes) -> tuple[bytes, dict]:
    """Aplica a textura PT-BR na entrada Xbox e mede o conversor na versao inglesa."""
    lay = xbox_layout(xbox_entry)
    english = ps2_indices(ps2_english)
    h, w = english.shape
    fidelity = agreement(english, xbox_indices(xbox_entry)[:h, :w])
    # CLUT embutida: indices e paleta PT-BR vao juntos (algumas dicas foram
    # refeitas no Xbox com outra ordem de cores). CLUT externa: paleta do Xbox.
    pal_en, pal_pt = ps2_palette(ps2_english), ps2_palette(ps2_ptbr)
    palette = None if pal_pt is None else xbox_palette_from_ps2(pal_pt, xbox_palette(xbox_entry))
    palette_check = None
    if pal_en is not None:
        expected = xbox_palette_from_ps2(pal_en, xbox_palette(xbox_entry))
        palette_check = float(np.mean(np.abs(expected.astype(int) - xbox_palette(xbox_entry).astype(int)) <= 2))
    out = write_xbox(xbox_entry, ps2_indices(ps2_ptbr), palette)
    return out, {'xbox_size': [lay['w'], lay['h']], 'ps2_size': [w, h], 'english_fidelity': round(fidelity, 4),
                 'palette_replaced': palette is not None,
                 'palette_check': None if palette_check is None else round(palette_check, 4)}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--ptbr-iso', type=Path, required=True)
    parser.add_argument('--orig-iso', type=Path, required=True)
    parser.add_argument('--xbox-data', type=Path, default=Path('assets/data'))
    parser.add_argument('--out-dir', type=Path, default=Path('out/build/release/mods/PTBR-PS2/files/data'))
    parser.add_argument('--report', type=Path, default=Path('.local/ptbr-ps2/textures.json'))
    args = parser.parse_args()

    orig = [blob for _, blob in walk_dars(read_iso_file(args.orig_iso, 'FILEDATA.BIN'))]
    ptbr = [blob for _, blob in walk_dars(read_iso_file(args.ptbr_iso, 'FILEDATA.BIN'))]
    if len(orig) != len(ptbr):
        raise ValueError('FILEDATA PT-BR com outra quantidade de DARs')
    xbox_files, xbox_where, english_ids, xbox_textures = {}, {}, set(), {}
    for path in sorted(args.xbox_data.glob('*.dar')):
        blob = path.read_bytes()
        if blob[:4] != DAR_MAGIC:
            continue
        xbox_files[path.name] = bytearray(blob)
        for entry_id, kind, start, size in dar_records(blob):
            if kind != TEXTURE:
                continue
            xbox_where.setdefault(entry_id, []).append((path.name, start, size))
            if english_dar(path.name):
                english_ids.add(entry_id)
            if entry_id not in xbox_textures:
                try:
                    xbox_textures[entry_id] = xbox_indices(blob[start:start + size])
                except ValueError:
                    pass  # outros formatos D3D nao entram na busca
    english_of = {}
    for blob in orig:  # textura inglesa PS2 por ID
        for entry_id, kind, start, size in dar_records(blob):
            if kind == TEXTURE:
                english_of.setdefault(entry_id, blob[start:start + size])

    report, touched, problems = [], set(), []
    for item in changed_textures(orig, ptbr):
        english_id, score, method = english_counterpart(item, orig, set(xbox_where), english_ids, xbox_textures)
        row = {'dar': item['dar'], 'id': item['id'], 'english_id': english_id, 'match': round(score, 4),
               'method': method}
        # Na posicao do grupo, texto em outra lingua derruba a semelhanca (telas de dica).
        weak = method == 'global_search' and score < (MIN_MATCH if english_id in english_ids else MIN_MATCH_OTHER)
        if english_id < 0 or weak or english_id not in english_of:
            problems.append(row)
            report.append(row)
            continue
        ps2_english = english_of[english_id] if english_id != item['id'] else item['orig']
        row['xbox'] = []
        for name, start, size in xbox_where[english_id]:
            entry = bytes(xbox_files[name][start:start + size])
            new, info = convert(ps2_english, item['ptbr'], entry)
            xbox_files[name][start:start + size] = new
            touched.add(name)
            row['xbox'].append({'file': name, **info})
        report.append(row)
        print(f'{item["dar"]:4d} {item["id"]:6d} -> {english_id:6d} ({score:.2f} {method}) '
              + ', '.join(f'{x["file"]} fid {x["english_fidelity"]:.3f}' for x in row['xbox']), flush=True)

    args.out_dir.mkdir(parents=True, exist_ok=True)
    for name in sorted(touched):
        (args.out_dir / name).write_bytes(bytes(xbox_files[name]))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({'textures': report, 'problems': problems,
                                       'files': {n: hashlib.sha256(xbox_files[n]).hexdigest() for n in sorted(touched)}},
                                      indent=1) + '\n', encoding='utf-8')
    print(f'{len(report) - len(problems)}/{len(report)} texturas; {len(touched)} DARs Xbox; problemas {len(problems)}')
    # Sem par fica no relatorio: 16309 e uma dica so do PAL (cristais SIM/NAO).
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
