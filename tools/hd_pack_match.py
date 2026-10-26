"""Pareia um pacote de texturas do PCSX2 (PNG com nomes do PS2) com as texturas do Xbox.

  python tools/hd_pack_match.py --pack <pasta replacements> --assets assets/data --out .local/hd4k/match.json
  python tools/hd_pack_match.py build --match .local/hd4k/match.json --out out/build/release/mods/HD4K/textures
  python tools/hd_pack_match.py ptbr --pack <pasta replacements>
  python tools/hd_pack_match.py buttons --pack <pasta replacements>

O runtime procura cada textura HD pelo hash da textura Xbox (veja dar_texture.pack_hash).
O pareamento reduz cada PNG e compara a imagem com as texturas P8 dos DARs: mesma proporção,
menor distância de cor, com folga para o segundo colocado. O build converte os pares para
DDS BC7 com mipmaps (texconv) e grava <hash>.dds; o mesmo PNG para vários hashes vira links.
O ptbr monta o 4K das texturas traduzidas: 4K inglês onde nada mudou, PT-BR ampliada no texto.
"""
from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import tempfile
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

try:
    from dar_texture import entry_rgba, pack_hash
    from ps2_ptbr_import import DAR_MAGIC
    from ps2_texture_import import TEXTURE, dar_records
except ImportError:  # unittest a partir da raiz
    from tools.dar_texture import entry_rgba, pack_hash
    from tools.ps2_ptbr_import import DAR_MAGIC
    from tools.ps2_texture_import import TEXTURE, dar_records

COARSE, FINE = 8, 32
# Limites conferidos em imagem (25/10/2026): até 6/16 todos certos; acima, entram página do
# bestiário com outro monstro, pose diferente e variante de cor (olho vermelho x azul).
MAX_DISTANCE = 6.0    # média por canal (0..255) na comparação fina 32x32
MAX_BLOCK = 16.0      # pior bloco 4x4 da comparação fina: pega diferença local que a média esconde
CANDIDATES = 16


def signature(rgba: np.ndarray, side: int) -> np.ndarray:
    """RGB pré-multiplicado pelo alpha + alpha, reduzido por média de área (float32, 0..255)."""
    image = Image.fromarray(np.ascontiguousarray(rgba, np.uint8), 'RGBA').resize((side, side), Image.Resampling.BOX)
    pixels = np.asarray(image, np.float32)
    alpha = pixels[..., 3:4] / 255.0
    return np.concatenate([pixels[..., :3] * alpha, pixels[..., 3:4]], axis=-1).reshape(-1)


def ps2_alpha_to_xbox(rgba: np.ndarray) -> np.ndarray:
    """PCSX2 grava o alpha do PS2 (0x80 = opaco); o Xbox usa 0..255, como a conversão da paleta."""
    out = np.array(rgba, np.uint8, copy=True)
    out[..., 3] = np.minimum(255, out[..., 3].astype(np.uint16) * 2).astype(np.uint8)
    return out


def png_signature(path: str) -> tuple[str, int, int, list[float], list[float]] | None:
    try:
        with Image.open(path) as image:
            width, height = image.size
            image = image.convert('RGBA')
            factor = max(1, min(width, height) // 64)
            if factor > 1:
                image = image.reduce(factor)
            rgba = ps2_alpha_to_xbox(np.asarray(image))
    except OSError:
        return None
    return (Path(path).name, width, height, signature(rgba, COARSE).tolist(), signature(rgba, FINE).tolist())


def xbox_textures(assets: Path) -> dict[int, dict]:
    """Texturas P8 com paleta própria, únicas por hash do runtime."""
    found: dict[int, dict] = {}
    for dar in sorted(assets.rglob('*.dar')):
        blob = dar.read_bytes()
        if blob[:4] != DAR_MAGIC:
            continue
        for entry_id, kind, start, size in dar_records(blob):
            if kind != TEXTURE:
                continue
            entry = blob[start:start + size]
            try:
                rgba = entry_rgba(entry)
                key = pack_hash(entry)
            except ValueError:   # 1x1 sem formato ou paleta externa
                continue
            if key in found:
                continue
            height, width = rgba.shape[:2]
            found[key] = {'dar': str(dar.relative_to(assets)), 'id': entry_id, 'w': width, 'h': height,
                          'coarse': signature(rgba, COARSE), 'fine': signature(rgba, FINE)}
    return found


def worst_block(diff: np.ndarray) -> np.ndarray:
    """Maior média de bloco 4x4 (todos os canais) em diferenças de assinaturas FINE (n, 32*32*4)."""
    blocks = diff.reshape(-1, FINE // 4, 4, FINE // 4, 4, 4).mean(axis=(2, 4, 5))
    return blocks.reshape(len(diff), -1).max(axis=1)


def match(xbox: dict[int, dict], pngs: list[tuple]) -> dict:
    """Melhor PNG para cada textura Xbox. Aceita se a média e o pior bloco ficam nos limites e
    nenhum outro candidato plausível é uma imagem diferente (ex.: variante de cor parecida)."""
    names = [p[0] for p in pngs]
    ratio = np.array([p[1] / p[2] for p in pngs])
    sizes = np.array([(p[1], p[2]) for p in pngs])
    coarse = np.array([p[3] for p in pngs], np.float32)
    fine = np.array([p[4] for p in pngs], np.float32)
    pairs, rejected = {}, {}
    for key, tex in xbox.items():
        ok = np.flatnonzero((np.abs(ratio - tex['w'] / tex['h']) < 1e-6) & (sizes[:, 0] >= tex['w']))
        if not len(ok):
            rejected[f'{key:016X}'] = {'motivo': 'sem PNG na mesma proporção', **_where(tex)}
            continue
        near = ok[np.argsort(np.abs(coarse[ok] - tex['coarse']).mean(axis=1))[:CANDIDATES]]
        diff = np.abs(fine[near] - tex['fine'])
        distance, block = diff.mean(axis=1), worst_block(diff)
        best = int(np.argmin(distance))
        plausible = np.flatnonzero((distance <= MAX_DISTANCE) & (block <= MAX_BLOCK))
        # outro candidato plausível que é imagem diferente do melhor: não dá para escolher
        rivals = [i for i in plausible if i != best and
                  worst_block(np.abs(fine[near[i]] - fine[near[best]])[None])[0] > MAX_BLOCK]
        record = {'png': names[near[best]], 'distancia': round(float(distance[best]), 2),
                  'bloco': round(float(block[best]), 2), **_where(tex)}
        if best not in plausible:
            rejected[f'{key:016X}'] = {'motivo': 'imagem diferente', **record}
        elif rivals:
            rejected[f'{key:016X}'] = {'motivo': 'ambígua', 'rival': names[near[rivals[0]]], **record}
        else:
            pairs[f'{key:016X}'] = record
    used = {r['png'] for r in pairs.values()}
    return {'pares': pairs, 'rejeitados': rejected, 'png_sem_par': sorted(set(names) - used)}


def _where(tex: dict) -> dict:
    return {'dar': tex['dar'], 'id': tex['id'], 'w': tex['w'], 'h': tex['h']}


def scan(args) -> int:
    pack = sorted(str(p) for p in args.pack.glob('*.png'))
    cache = args.out.with_name('png_signatures_v2.json')   # v2: alpha do PS2 convertido
    known = {}
    if cache.exists():
        known = {item[0]: item for item in json.loads(cache.read_text())}
    todo = [p for p in pack if Path(p).name not in known]
    if todo:
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            for done, item in enumerate(pool.map(png_signature, todo, chunksize=8), 1):
                if item:
                    known[item[0]] = item
                if done % 250 == 0:
                    print(f'PNG {done}/{len(todo)}', flush=True)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        cache.write_text(json.dumps(list(known.values())))
    pngs = [known[Path(p).name] for p in pack if Path(p).name in known]
    xbox = xbox_textures(args.assets)
    result = match(xbox, pngs)
    result['resumo'] = {'png': len(pngs), 'xbox_unicas': len(xbox), 'pares': len(result['pares']),
                        'rejeitados': len(result['rejeitados']), 'png_sem_par': len(result['png_sem_par'])}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(result, indent=1, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(result['resumo'], ensure_ascii=False))
    return 0


def compose_translation(english_4k: np.ndarray, english: np.ndarray, translated: np.ndarray,
                        threshold: int = 24) -> tuple[np.ndarray, float]:
    """4K da tradução: o 4K inglês onde a textura PT-BR é igual à inglesa; a PT-BR ampliada onde
    mudou (o texto), com borda suave. Devolve a imagem e a fração da área trocada."""
    h, w = english.shape[:2]
    big_h, big_w = english_4k.shape[:2]
    changed = (np.abs(translated.astype(np.int16) - english.astype(np.int16)) > threshold).any(axis=-1)
    mask = Image.fromarray((changed * 255).astype(np.uint8), 'L').filter(ImageFilter.MaxFilter(3))
    soft = np.asarray(mask.resize((big_w, big_h), Image.Resampling.BILINEAR), np.float32)[..., None] / 255.0
    text = np.asarray(Image.fromarray(translated, 'RGBA').resize((big_w, big_h), Image.Resampling.LANCZOS), np.float32)
    out = english_4k.astype(np.float32) * (1 - soft) + text * soft
    return np.clip(out + 0.5, 0, 255).astype(np.uint8), float(changed.mean()) if h and w else 0.0


NEAR_DISTANCE = 10.0   # média do melhor candidato rejeitado que ainda vale tentar compor
NEAR_MAX_AREA = 0.5    # acima disso a textura sai quase toda ampliada: não vale trocar a original


def premultiplied(rgba: np.ndarray) -> np.ndarray:
    pixels = rgba.astype(np.float32)
    return np.concatenate([pixels[..., :3] * pixels[..., 3:4] / 255.0, pixels[..., 3:4]], axis=-1)


def enlarge(rgba: np.ndarray, size: tuple[int, int]) -> np.ndarray:
    """Amplia cada canal em float (bilinear), sem arredondar."""
    return np.stack([np.asarray(Image.fromarray(rgba[..., c].astype(np.float32), 'F').resize(size, Image.Resampling.BILINEAR))
                     for c in range(rgba.shape[-1])], axis=-1)


def compose_near(pack_4k: np.ndarray, xbox: np.ndarray, threshold: int = 24) -> tuple[np.ndarray, float]:
    """Quase igual: original ampliada + só o detalhe fino do 4K (o 4K menos a própria versão
    reduzida e ampliada) onde, no tamanho do Xbox, o 4K bate com a original; onde difere (ícone de
    botão, outra pose) fica só a original ampliada, com borda suave. Cor e forma sempre vêm da
    original, então a aura fraca de outro monstro não vira mancha. Compara com alpha
    pré-multiplicado para pixel invisível não contar. Devolve a imagem e a fração trocada."""
    h, w = xbox.shape[:2]
    big_h, big_w = pack_4k.shape[:2]
    reduced = np.asarray(Image.fromarray(pack_4k, 'RGBA').resize((w, h), Image.Resampling.BOX))
    changed = (np.abs(premultiplied(reduced) - premultiplied(xbox)) > threshold).any(axis=-1)
    mask = Image.fromarray((changed * 255).astype(np.uint8), 'L').filter(ImageFilter.MaxFilter(3))
    soft = np.asarray(mask.resize((big_w, big_h), Image.Resampling.BILINEAR), np.float32)[..., None] / 255.0
    detail = pack_4k.astype(np.float32) - enlarge(reduced, (big_w, big_h))
    out = enlarge(xbox, (big_w, big_h)) + detail * (1 - soft)
    return np.clip(out + 0.5, 0, 255).astype(np.uint8), float(changed.mean())


# Atlas de ícones global.dar#16020: as células (x0, y0, x1, y1 no tamanho do Xbox, fim exclusivo) dos botões
# B Y X / A L preto R branco, que no pack 4K do PS2 são ○ △ □ / ✕ L1 L2 R1 R2, e a da estátua Moai (bônus do
# save de Lament of Innocence), vazia no Xbox. O resto do atlas (ícones de item) tem outras cores no 4K e fica
# com a original ampliada.
PS2_BUTTONS_HASH = '3578834CA54C2A49'
PS2_BUTTONS_DAR = ('global.dar', 16020)
PS2_BUTTON_CELLS = ((168, 182, 192, 208), (192, 182, 216, 208), (216, 182, 240, 208),
                    (0, 208, 24, 236), (24, 208, 48, 236), (48, 208, 72, 236), (72, 208, 96, 236), (96, 208, 120, 236),
                    (24, 129, 48, 157))


def compose_cells(pack_4k: np.ndarray, xbox: np.ndarray, cells) -> np.ndarray:
    """Original ampliada, com as células escolhidas copiadas do 4K (escala inteira, mesma proporção)."""
    h, w = xbox.shape[:2]
    big_h, big_w = pack_4k.shape[:2]
    if big_w % w or big_h % h or big_w // w != big_h // h:
        raise ValueError(f'4K {big_w}x{big_h} não é múltiplo inteiro de {w}x{h}')
    scale = big_w // w
    out = np.clip(enlarge(xbox, (big_w, big_h)) + 0.5, 0, 255).astype(np.uint8)
    for x0, y0, x1, y1 in cells:
        out[y0 * scale:y1 * scale, x0 * scale:x1 * scale] = pack_4k[y0 * scale:y1 * scale, x0 * scale:x1 * scale]
    return out


def buttons(args) -> int:
    """Pack PS2-Botoes: atlas de ícones com os botões do PS2, gravado como <hash>.dds."""
    result = json.loads(args.match.read_text(encoding='utf-8'))
    record = result['pares'].get(PS2_BUTTONS_HASH) or result['rejeitados'].get(PS2_BUTTONS_HASH)
    if not record or 'png' not in record:
        print(f'{PS2_BUTTONS_HASH} sem PNG do 4K no relatório {args.match}')
        return 1
    xbox = xbox_entry(args.assets, *PS2_BUTTONS_DAR, {})
    if xbox is None or pack_hash_of(args.assets, *PS2_BUTTONS_DAR) != int(PS2_BUTTONS_HASH, 16):
        print('atlas de ícones do Xbox não encontrado ou com outro hash')
        return 1
    with Image.open(args.pack / record['png']) as image:
        pack_4k = ps2_alpha_to_xbox(np.asarray(image.convert('RGBA')))
    composed = compose_cells(pack_4k, xbox, PS2_BUTTON_CELLS)
    args.out.mkdir(parents=True, exist_ok=True)
    if args.preview:
        args.preview.parent.mkdir(parents=True, exist_ok=True)
        Image.fromarray(composed, 'RGBA').save(args.preview, compress_level=1)
    with tempfile.TemporaryDirectory() as tmp:
        source = Path(tmp) / f'{PS2_BUTTONS_HASH}.png'
        Image.fromarray(composed, 'RGBA').save(source, compress_level=1)
        run = subprocess.run(texconv_command(args.texconv, source, Path(tmp)), capture_output=True, text=True)
        converted = Path(tmp) / f'{PS2_BUTTONS_HASH}.dds'
        if run.returncode or not converted.exists():
            print(f'texconv falhou: {run.stdout.strip()[-200:]}')
            return 1
        shutil.move(converted, args.out / f'{PS2_BUTTONS_HASH}.dds')
    print(f'PS2-Botoes: {args.out / (PS2_BUTTONS_HASH + ".dds")}')
    return 0


def pack_hash_of(assets: Path, dar_name: str, entry_id: int) -> int | None:
    blob = (assets / dar_name).read_bytes()
    for eid, kind, start, size in dar_records(blob):
        if eid == entry_id and kind == TEXTURE:
            return pack_hash(blob[start:start + size])
    return None


def xbox_entry(assets: Path, dar_name: str, entry_id: int, cache: dict) -> np.ndarray | None:
    blob = cache.setdefault(dar_name, (assets / dar_name).read_bytes())
    for eid, kind, start, size in dar_records(blob):
        if eid == entry_id and kind == TEXTURE:
            return entry_rgba(blob[start:start + size])
    return None


def near(args) -> int:
    """Rejeitadas por diferença pequena: composição 4K + original ampliada, gravada no HD4K."""
    rejected = json.loads(args.match.read_text(encoding='utf-8'))['rejeitados']
    args.out.mkdir(parents=True, exist_ok=True)
    report, made, cache = [], 0, {}
    candidates = sorted((key, r) for key, r in rejected.items()
                        if r['motivo'] == 'imagem diferente' and r['distancia'] <= args.max_distance)
    with tempfile.TemporaryDirectory() as tmp:
        for key, record in candidates:
            item = {'hash': key, 'dar': record['dar'], 'id': record['id'], 'png': record['png'],
                    'distancia': record['distancia']}
            if (args.out / f'{key}.dds').exists():
                report.append({**item, 'status': 'já existe (mantido)'})
                continue
            xbox = xbox_entry(args.assets, record['dar'], record['id'], cache)
            if xbox is None:
                report.append({**item, 'status': 'textura não encontrada'})
                continue
            with Image.open(args.pack / record['png']) as image:
                pack_4k = ps2_alpha_to_xbox(np.asarray(image.convert('RGBA')))
            composed, area = compose_near(pack_4k, xbox)
            item['area_trocada'] = round(area, 3)
            if area > args.max_area:
                report.append({**item, 'status': 'diferente demais (original mantida)'})
                continue
            if args.preview:
                args.preview.mkdir(parents=True, exist_ok=True)
                Image.fromarray(composed, 'RGBA').save(args.preview / f'{key}.png', compress_level=1)
            if args.dry_run:
                report.append({**item, 'status': 'aprovada (simulação)'})
                continue
            source = Path(tmp) / f'{key}.png'
            Image.fromarray(composed, 'RGBA').save(source, compress_level=1)
            run = subprocess.run(texconv_command(args.texconv, source, Path(tmp)), capture_output=True, text=True)
            converted = Path(tmp) / f'{key}.dds'
            source.unlink(missing_ok=True)
            if run.returncode or not converted.exists():
                report.append({**item, 'status': 'texconv falhou'})
                continue
            shutil.move(converted, args.out / f'{key}.dds')
            report.append({**item, 'status': 'feito'})
            made += 1
            if made % 50 == 0:
                print(f'compostas {made}', flush=True)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=1, ensure_ascii=False), encoding='utf-8')
    approved = sum(r['status'] in ('feito', 'aprovada (simulação)') for r in report)
    print(f'Quase iguais: {approved} aprovadas, {made} DDS gravados, de {len(candidates)} candidatas; relatório em {args.report}')
    return 0


def ptbr(args) -> int:
    """Texturas PT-BR em 4K, a partir dos pares do 4K inglês, gravadas em PTBR-PS2/textures."""
    pairs = json.loads(args.match.read_text(encoding='utf-8'))['pares']
    args.out.mkdir(parents=True, exist_ok=True)
    report, made = [], 0
    with tempfile.TemporaryDirectory() as tmp:
        for mod_dar in sorted(args.mod_data.glob('*.dar')):
            original = args.assets / mod_dar.name
            if not original.exists():
                continue
            mod_blob, orig_blob = mod_dar.read_bytes(), original.read_bytes()
            orig_entries = {eid: orig_blob[s:s + z] for eid, kind, s, z in dar_records(orig_blob) if kind == TEXTURE}
            for entry_id, kind, start, size in dar_records(mod_blob):
                translated_entry = mod_blob[start:start + size]
                english_entry = orig_entries.get(entry_id)
                if kind != TEXTURE or english_entry is None or english_entry == translated_entry:
                    continue
                try:
                    english_key, translated_key = pack_hash(english_entry), pack_hash(translated_entry)
                    english, translated = entry_rgba(english_entry), entry_rgba(translated_entry)
                except ValueError:
                    continue
                name = f'{translated_key:016X}'
                pair = pairs.get(f'{english_key:016X}')
                item = {'dar': mod_dar.name, 'id': entry_id, 'hash': name}
                if any((args.out / f'{name}{ext}').exists() for ext in ('.bmp', '.dds')):
                    report.append({**item, 'status': 'já existe (mantido)'})
                    continue
                if not pair:
                    report.append({**item, 'status': 'sem 4K inglês'})
                    continue
                with Image.open(args.pack / pair['png']) as image:
                    english_4k = ps2_alpha_to_xbox(np.asarray(image.convert('RGBA')))
                composed, area = compose_translation(english_4k, english, translated)
                source = Path(tmp) / f'{name}.png'
                Image.fromarray(composed, 'RGBA').save(source, compress_level=1)
                run = subprocess.run(texconv_command(args.texconv, source, Path(tmp)), capture_output=True, text=True)
                converted = Path(tmp) / f'{name}.dds'
                if run.returncode or not converted.exists():
                    report.append({**item, 'status': 'texconv falhou'})
                    continue
                shutil.move(converted, args.out / f'{name}.dds')
                source.unlink(missing_ok=True)
                report.append({**item, 'status': 'feito', 'area_trocada': round(area, 3)})
                made += 1
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=1, ensure_ascii=False), encoding='utf-8')
    print(f'PT-BR 4K: {made} feitas de {len(report)} texturas traduzidas; relatório em {args.report}')
    return 0


def texconv_command(texconv: Path, png: Path, out: Path) -> list[str]:
    return [str(texconv), '-nologo', '-f', 'BC7_UNORM', '-m', '0', '--ignore-srgb', '-sepalpha', '-y',
            '-o', str(out), str(png)]


def build(args) -> int:
    result = json.loads(args.match.read_text(encoding='utf-8'))
    by_png: dict[str, list[str]] = {}
    for key, record in result['pares'].items():
        by_png.setdefault(record['png'], []).append(key)
    args.out.mkdir(parents=True, exist_ok=True)
    done = 0
    with tempfile.TemporaryDirectory() as tmp:
        for png, keys in sorted(by_png.items()):
            targets = [args.out / f'{key}.dds' for key in keys]
            if all(t.exists() for t in targets):
                done += 1
                continue
            source = Path(tmp) / png
            with Image.open(args.pack / png) as image:
                Image.fromarray(ps2_alpha_to_xbox(np.asarray(image.convert('RGBA'))), 'RGBA').save(source, compress_level=1)
            converted = Path(tmp) / (Path(png).stem + '.dds')
            run = subprocess.run(texconv_command(args.texconv, source, Path(tmp)), capture_output=True, text=True)
            source.unlink(missing_ok=True)
            if run.returncode or not converted.exists():
                print(f'falhou {png}: {run.stdout.strip()[-200:]}', flush=True)
                continue
            first = targets[0]
            shutil.move(converted, first)
            for other in targets[1:]:   # mesmo PNG para outro hash: link no NTFS, cópia se não der
                other.unlink(missing_ok=True)
                try:
                    os.link(first, other)
                except OSError:
                    shutil.copy2(first, other)
            done += 1
            if done % 100 == 0:
                print(f'DDS {done}/{len(by_png)}', flush=True)
    print(f'DDS prontos: {done}/{len(by_png)} PNGs, {len(result["pares"])} hashes')
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command')
    parser.add_argument('--pack', type=Path)
    parser.add_argument('--assets', type=Path, default=Path('assets/data'))
    parser.add_argument('--out', type=Path, default=Path('.local/hd4k/match.json'))
    parser.add_argument('--jobs', type=int, default=max(1, (os.cpu_count() or 2) - 2))
    make = sub.add_parser('build')
    make.add_argument('--match', type=Path, default=Path('.local/hd4k/match.json'))
    make.add_argument('--pack', type=Path, required=True)
    make.add_argument('--out', type=Path, default=Path('out/build/release/mods/HD4K/textures'))
    make.add_argument('--texconv', type=Path, default=Path('.local/build-tools/texconv.exe'))
    translate = sub.add_parser('ptbr')
    translate.add_argument('--match', type=Path, default=Path('.local/hd4k/match.json'))
    translate.add_argument('--pack', type=Path, required=True)
    translate.add_argument('--assets', type=Path, default=Path('assets/data'))
    translate.add_argument('--mod-data', type=Path, default=Path('out/build/release/mods/PTBR-PS2/files/data'))
    translate.add_argument('--out', type=Path, default=Path('out/build/release/mods/PTBR-PS2/textures'))
    translate.add_argument('--report', type=Path, default=Path('.local/hd4k/ptbr.json'))
    translate.add_argument('--texconv', type=Path, default=Path('.local/build-tools/texconv.exe'))
    close = sub.add_parser('near')
    close.add_argument('--match', type=Path, default=Path('.local/hd4k/match.json'))
    close.add_argument('--pack', type=Path, required=True)
    close.add_argument('--assets', type=Path, default=Path('assets/data'))
    close.add_argument('--out', type=Path, default=Path('out/build/release/mods/HD4K/textures'))
    close.add_argument('--report', type=Path, default=Path('.local/hd4k/near.json'))
    close.add_argument('--texconv', type=Path, default=Path('.local/build-tools/texconv.exe'))
    close.add_argument('--max-distance', type=float, default=NEAR_DISTANCE)
    close.add_argument('--max-area', type=float, default=NEAR_MAX_AREA)
    close.add_argument('--preview', type=Path, help='grava também o PNG composto nesta pasta')
    close.add_argument('--dry-run', action='store_true', help='só calcula e relata, sem gravar DDS')
    icons = sub.add_parser('buttons')
    icons.add_argument('--match', type=Path, default=Path('.local/hd4k/match.json'))
    icons.add_argument('--pack', type=Path, required=True)
    icons.add_argument('--assets', type=Path, default=Path('assets/data'))
    icons.add_argument('--out', type=Path, default=Path('out/build/release/mods/PS2-Botoes/textures'))
    icons.add_argument('--texconv', type=Path, default=Path('.local/build-tools/texconv.exe'))
    icons.add_argument('--preview', type=Path, help='grava também o PNG composto neste arquivo')
    args = parser.parse_args()
    if args.command == 'buttons':
        return buttons(args)
    if args.command == 'build':
        return build(args)
    if args.command == 'near':
        return near(args)
    if args.command == 'ptbr':
        return ptbr(args)
    if not args.pack:
        parser.error('--pack é obrigatório')
    return scan(args)


if __name__ == '__main__':
    raise SystemExit(main())
