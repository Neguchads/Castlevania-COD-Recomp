"""Exporta e reimporta uma textura P8 de DARs Xbox para edição manual.

  python tools/dar_texture.py export --id 14452 --dar <mod>/files/data/titleue.dar --png titulo.png
  python tools/dar_texture.py import --id 14452 --png titulo.png --dir <mod>/files/data
  python tools/dar_texture.py hd --id 14452 --dar <mod>/files/data/titleue.dar --png remake.png

O hd grava a imagem em qualquer resolução (até 16384) como mods/PTBR-PS2/textures/<hash>.bmp:
o runtime troca a textura na GPU pelo hash da textura que o jogo carrega do DAR.

O import grava em todos os DARs da pasta que têm o ID e guarda o DAR anterior em
--backup. Se a imagem usa só cores da paleta atual, a paleta fica igual; senão a
imagem é reduzida a 256 cores e a paleta é trocada.
"""
from __future__ import annotations

import argparse
import shutil
import struct
from pathlib import Path

import numpy as np
from PIL import Image

try:
    from ps2_texture_import import DAR_MAGIC, TEXTURE, dar_records, write_xbox, xbox_indices, xbox_layout, xbox_palette
except ImportError:  # unittest a partir da raiz
    from tools.ps2_texture_import import (DAR_MAGIC, TEXTURE, dar_records, write_xbox, xbox_indices,
                                          xbox_layout, xbox_palette)


def own_palette(entry: bytes) -> np.ndarray:
    """Paleta da própria entrada. Texturas de 16 cores com CLUT externa (ex.: máscaras
    do logo 15355–15358, CLUT 14451) têm a área vazia e não podem ser editadas aqui."""
    pal = xbox_palette(entry)
    if not pal.any():
        raise ValueError('Textura usa paleta externa (16 cores); não é editável por esta ferramenta')
    return pal


def entry_rgba(entry: bytes) -> np.ndarray:
    """Pixels RGBA (alpha 0..255) da textura P8 Xbox."""
    pal = own_palette(entry)
    rgba = np.stack([pal[:, 2], pal[:, 1], pal[:, 0], pal[:, 3]], axis=1)
    return rgba[xbox_indices(entry)].astype(np.uint8)


def exact_indices(rgba: np.ndarray, palette_rgba: np.ndarray) -> np.ndarray | None:
    """Índices na paleta atual, se toda cor da imagem já existir nela."""
    key = lambda a: (a[..., 0].astype(np.uint32) << 24 | a[..., 1].astype(np.uint32) << 16
                     | a[..., 2].astype(np.uint32) << 8 | a[..., 3].astype(np.uint32))
    codes, first = np.unique(key(palette_rgba), return_index=True)
    image = key(rgba)
    pos = np.searchsorted(codes, image).clip(0, len(codes) - 1)
    if not np.all(codes[pos] == image):
        return None
    return first[pos].astype(np.uint8)


def entry_with_rgba(entry: bytes, rgba: np.ndarray) -> tuple[bytes, bool]:
    """Nova entrada com a imagem; True quando a paleta precisou mudar."""
    lay = xbox_layout(entry)
    if rgba.shape[:2] != (lay['h'], lay['w']):
        raise ValueError(f'Imagem {rgba.shape[1]}x{rgba.shape[0]}, textura {lay["w"]}x{lay["h"]}')
    pal = own_palette(entry)
    indices = exact_indices(rgba, np.stack([pal[:, 2], pal[:, 1], pal[:, 0], pal[:, 3]], axis=1))
    if indices is not None:
        return write_xbox(entry, indices, None), False
    quant = Image.fromarray(rgba, 'RGBA').quantize(256, method=Image.Quantize.FASTOCTREE)
    colors = np.zeros((256, 4), np.uint8)
    raw = np.array(quant.getpalette('RGBA'), np.uint8).reshape(-1, 4)[:256]
    colors[:len(raw)] = raw
    bgra = colors[:, [2, 1, 0, 3]]
    return write_xbox(entry, np.asarray(quant, np.uint8), bgra), True


_MASK, _K0, _K1 = (1 << 64) - 1, 0x9E3779B97F4A7C15, 0xE7037ED1A0B428DB


def runtime_hash(data: bytes, seed: int = 1469598103934665603) -> int:
    """Cópia de Hash() em XboxXBERecomp/runtime/gpu/shaders.cpp."""
    def mix(a, b):
        r = a * b
        return (r & _MASK) ^ (r >> 64)
    n, h, i = len(data), seed ^ _K0, 0
    while i + 16 <= n:
        x, y = struct.unpack_from('<QQ', data, i)
        h = mix(x ^ _K1, y ^ h)
        i += 16
    if r := n - i:
        if r >= 8:
            x, y = struct.unpack_from('<Q', data, i)[0], struct.unpack_from('<Q', data, n - 8)[0]
        else:
            x, y = int.from_bytes(data[i:n], 'little'), 0
        h = mix(x ^ _K1, y ^ h ^ r)
    return mix(h ^ _K0, n ^ _K1)


def pack_hash(entry: bytes) -> int:
    """Nome do arquivo HD: como BindGuestTexture em src/common/d3d12_runtime.inc
    (pixels, formato P8, largura, altura; XOR hash da paleta de 256 cores)."""
    lay = xbox_layout(entry)
    pixels = entry[0x100:0x100 + lay['w'] * lay['h']]
    h = runtime_hash(pixels)
    for value in (0x0B, lay['w'], lay['h']):
        h = runtime_hash(struct.pack('<I', value), h)
    return h ^ runtime_hash(entry[lay['palette']:lay['palette'] + 1024])


def write_bmp32(path: Path, rgba: np.ndarray) -> None:
    """BMP 32 bits de cima para baixo (altura negativa), como DecodePackBmp lê."""
    h, w = rgba.shape[:2]
    if max(w, h) > 16384:
        raise ValueError('Lado maior que 16384 (limite do pacote HD)')
    bgra = np.ascontiguousarray(rgba[..., [2, 1, 0, 3]], np.uint8).tobytes()
    header = struct.pack('<2sIHHI', b'BM', 54 + len(bgra), 0, 0, 54)
    info = struct.pack('<IiiHHIIiiII', 40, w, -h, 1, 32, 0, len(bgra), 2835, 2835, 0, 0)
    path.write_bytes(header + info + bgra)


def texture_records(blob: bytes, entry_id: int) -> list[tuple[int, int]]:
    return [(start, size) for eid, kind, start, size in dar_records(blob) if eid == entry_id and kind == TEXTURE]


def export(args) -> int:
    blob = args.dar.read_bytes()
    found = texture_records(blob, args.id)
    if not found:
        raise SystemExit(f'ID {args.id} não está em {args.dar}')
    start, size = found[0]
    Image.fromarray(entry_rgba(blob[start:start + size]), 'RGBA').save(args.png)
    print(f'{args.png}: ID {args.id} de {args.dar.name} (offset {start}, {size} bytes)')
    return 0


def import_(args) -> int:
    rgba = np.asarray(Image.open(args.png).convert('RGBA'))
    changed = 0
    for path in sorted(args.dir.glob('*.dar')):
        blob = bytearray(path.read_bytes())
        if blob[:4] != DAR_MAGIC:
            continue
        found = texture_records(bytes(blob), args.id)
        if not found:
            continue
        args.backup.mkdir(parents=True, exist_ok=True)
        if not (args.backup / path.name).exists():
            shutil.copy2(path, args.backup / path.name)
        repalette = False
        for start, size in found:
            new, changed_palette = entry_with_rgba(bytes(blob[start:start + size]), rgba)
            blob[start:start + size] = new
            repalette = repalette or changed_palette
        path.write_bytes(blob)
        changed += 1
        print(f'{path.name}: ID {args.id} atualizado' + (' (paleta nova, 256 cores)' if repalette else ''))
    if not changed:
        raise SystemExit(f'ID {args.id} não está em nenhum DAR de {args.dir}')
    return 0


def hd(args) -> int:
    blob = args.dar.read_bytes()
    found = texture_records(blob, args.id)
    if not found:
        raise SystemExit(f'ID {args.id} não está em {args.dar}')
    start, size = found[0]
    entry = blob[start:start + size]
    own_palette(entry)  # CLUT externa: hash depende de outra paleta
    name = f'{pack_hash(entry):016X}.bmp'
    args.out.mkdir(parents=True, exist_ok=True)
    rgba = np.asarray(Image.open(args.png).convert('RGBA'))
    write_bmp32(args.out / name, rgba)
    print(f'{args.out / name}: ID {args.id} ({rgba.shape[1]}x{rgba.shape[0]})')
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    exp = sub.add_parser('export')
    exp.add_argument('--id', type=int, required=True)
    exp.add_argument('--dar', type=Path, required=True)
    exp.add_argument('--png', type=Path, required=True)
    imp = sub.add_parser('import')
    imp.add_argument('--id', type=int, required=True)
    imp.add_argument('--png', type=Path, required=True)
    imp.add_argument('--dir', type=Path, default=Path('out/build/release/mods/PTBR-PS2/files/data'))
    imp.add_argument('--backup', type=Path, default=Path('.local/ptbr-ps2/dar-backup'))
    high = sub.add_parser('hd')
    high.add_argument('--id', type=int, required=True)
    high.add_argument('--dar', type=Path, required=True)
    high.add_argument('--png', type=Path, required=True)
    high.add_argument('--out', type=Path, default=Path('out/build/release/mods/PTBR-PS2/textures'))
    args = parser.parse_args()
    return {'export': export, 'import': import_, 'hd': hd}[args.command](args)


if __name__ == '__main__':
    raise SystemExit(main())
