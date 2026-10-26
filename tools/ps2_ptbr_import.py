"""Gera o catalogo PT-BR privado a partir da ISO PS2 PAL traduzida (SLES-53755).

O patch PT-BR ocupa o slot espanhol do PAL: tabela 14410. As tabelas PS2 e Xbox
usam o mesmo indice de texto, entao cada ID Xbox nao vazio recebe a string PT-BR
do mesmo ID, com os ajustes que o Xbox exige (tokens, titulo colorido). Nenhum
arquivo do jogo e alterado; a saida e o header privado lido pelo hook de textos.

Uso:
  python tools/ps2_ptbr_import.py --iso <ISO PT-BR> \
      [--xbox-dar assets/data/strdataue.dar] [--out assets/ptbr_strings.private.h] \
      [--fallback assets/ptbr_strings.private.h] [--report .local/ptbr-ps2/report.json]
"""

import argparse
import collections
import hashlib
import json
import re
import struct
from pathlib import Path

try:
    from ptbr_catalog import TOKEN_GROUPS, audit, read_catalog
except ModuleNotFoundError:
    from tools.ptbr_catalog import TOKEN_GROUPS, audit, read_catalog

SECTOR = 2048
DAR_MAGIC = bytes.fromhex('27497382')
EN_TABLE, PTBR_TABLE = 14405, 14410
NAMED = re.compile(r'\^<[^>]+>')
COLOR = re.compile(r'\^C[0-9A-Fa-f]{8}|\^c')
TITLE = re.compile(r'^\^C([0-9A-Fa-f]{8})(.*?)\^c\n?', re.S)
# Tags do PAL sem equivalente no Xbox: o texto literal e o que o Xbox mostra.
TAG_LITERALS = {'^<SANTEN>': '…', '^<Bel>': 'Belmont'}
# Nomes que o tradutor escreveu por extenso onde o Xbox usa a tag do nome.
NAME_TAGS = {'^<Bel>': 'Belmont', '^<Trevor>': 'Trevor'}
TOKEN = re.compile(r'\^<[^>]+>|\^C[0-9A-Fa-f]{8}|\^c|%(?:\d+)?[sdf]|\[(?:port|size)\]')


def iso_file(iso: Path, name: str) -> tuple[int, int]:
    """Offset e tamanho de um arquivo na raiz de uma ISO9660."""
    with iso.open('rb') as f:
        f.seek(16 * SECTOR)
        pvd = f.read(SECTOR)
        if pvd[1:6] != b'CD001':
            raise ValueError('ISO9660 nao encontrado')
        lba, size = struct.unpack_from('<I', pvd, 156 + 2)[0], struct.unpack_from('<I', pvd, 156 + 10)[0]
        f.seek(lba * SECTOR)
        directory = f.read(size)
    pos = 0
    while pos < len(directory):
        length = directory[pos]
        if length == 0:
            pos = (pos // SECTOR + 1) * SECTOR
            continue
        ident = directory[pos + 33:pos + 33 + directory[pos + 32]].decode('ascii', 'replace')
        if ident.split(';')[0].upper() == name.upper():
            return (struct.unpack_from('<I', directory, pos + 2)[0] * SECTOR,
                    struct.unpack_from('<I', directory, pos + 10)[0])
        pos += length
    raise FileNotFoundError(name)


def dar_entries(blob: bytes) -> dict[int, bytes]:
    """Entradas de um DAR; o offset e relativo ao registro da propria entrada."""
    count = struct.unpack_from('<I', blob, 0x10)[0]
    out = {}
    for index in range(count):
        record = 0x20 + 16 * index
        entry_id, _kind, offset, size = struct.unpack_from('<IIII', blob, record)
        out.setdefault(entry_id, blob[record + offset:record + offset + size])
    return out


def walk_dars(data: bytes):
    pos = SECTOR  # o primeiro setor do FILEDATA.BIN nao e DAR
    while pos + 0x20 <= len(data) and data[pos:pos + 4] == DAR_MAGIC:
        total = struct.unpack_from('<I', data, pos + 8)[0]
        yield pos, data[pos:pos + total]
        pos += (total + SECTOR - 1) // SECTOR * SECTOR


def table_strings(resource: bytes) -> list[bytes]:
    count = struct.unpack_from('<I', resource, 4)[0]
    out = []
    for index in range(count):
        offset = struct.unpack_from('<I', resource, 16 + 4 * index)[0]
        out.append(resource[offset:resource.index(b'\0', offset)])
    return out


def ps2_tables(iso: Path) -> tuple[list[bytes], list[bytes], dict]:
    offset, size = iso_file(iso, 'FILEDATA.BIN')
    with iso.open('rb') as f:
        f.seek(offset)
        data = f.read(size)
    copies = collections.defaultdict(set)
    for _pos, dar in walk_dars(data):
        entries = dar_entries(dar)
        for wanted in (EN_TABLE, PTBR_TABLE):
            if wanted in entries:
                copies[wanted].add(entries[wanted])
    info = {str(k): len(v) for k, v in copies.items()}
    merged = {}
    for wanted in (EN_TABLE, PTBR_TABLE):
        if not copies[wanted]:
            raise ValueError(f'Tabela {wanted} ausente na ISO')
        merged[wanted] = merge_copies([table_strings(c) for c in copies[wanted]], wanted)
    return merged[EN_TABLE], merged[PTBR_TABLE], {'distinct_copies': info,
                                                  'filedata_sha256': hashlib.sha256(data).hexdigest()}


def merge_copies(tables: list[list[bytes]], wanted: int) -> list[bytes]:
    """Une copias da mesma tabela. O patch esvaziou as legendas de cena numa copia
    (o PS2 tem legenda gravada no video); qualquer outra divergencia e erro."""
    size = len(tables[0])
    if any(len(t) != size for t in tables):
        raise ValueError(f'Tabela {wanted}: copias com contagens diferentes')
    out = []
    for index in range(size):
        texts = {t[index] for t in tables if t[index]}
        if len(texts) > 1:
            raise ValueError(f'Tabela {wanted}: versoes distintas no ID {index}')
        out.append(texts.pop() if texts else b'')
    return out


def xbox_strings(dar: bytes) -> list[bytes]:
    resource = 32 + struct.unpack_from('<I', dar, 40)[0]
    return table_strings(dar[resource:])


def tokens_match(original: str, text: str) -> bool:
    return all(p.findall(original) == p.findall(text) for p in TOKEN_GROUPS)


def adapt(xbox: str, ptbr: str, names: dict[str, str]) -> str:
    """Ajusta a string PT-BR ao que o Xbox espera para o mesmo ID."""
    text = ptbr
    xbox_tags = collections.Counter(NAMED.findall(xbox))
    for tag, literal in TAG_LITERALS.items():
        if tag in text and not xbox_tags[tag]:
            text = text.replace(tag, literal)
    for tag, literal in NAME_TAGS.items():
        missing = xbox_tags[tag] - NAMED.findall(text).count(tag)
        while missing > 0 and literal in text:
            text = text.replace(literal, tag, 1)
            missing -= 1
    if COLOR.findall(xbox) != COLOR.findall(text):
        title = TITLE.match(xbox)
        if title and not COLOR.findall(text):
            # Xbox poe o nome do item como titulo colorido antes da descricao.
            name = names.get(title.group(2), title.group(2))
            text = f'^C{title.group(1)}{name}^c\n' + text.lstrip(' ')
        elif not COLOR.findall(xbox):
            text = COLOR.sub('', text)
    return text


def build_catalog(xbox: list[bytes], english: list[bytes], ptbr: list[bytes],
                  fallback: dict[int, str]) -> tuple[dict[int, str], dict]:
    decoded_xbox = [s.decode('latin1') for s in xbox]
    pt_text = [s.decode('utf-8') for s in ptbr]
    # Nome em ingles do Xbox -> nome PT-BR do mesmo ID (para titulos coloridos).
    names = {}
    for index, text in enumerate(decoded_xbox):
        if text and index < len(pt_text) and pt_text[index] and '\n' not in text and '^' not in text:
            names.setdefault(text, pt_text[index])
    catalog, report = {}, collections.defaultdict(list)
    for index, original in enumerate(decoded_xbox):
        if not original:
            continue
        if not re.search(r'[A-Za-z]', TOKEN.sub('', original)):
            # So simbolos/tags/numeros (ex.: ^<DOLLAR>1): nada a traduzir, copia o original.
            try:
                catalog[index] = xbox[index].decode('utf-8')
            except UnicodeDecodeError:
                catalog[index] = original
            report['symbols_only_kept'].append(index)
            continue
        source = pt_text[index] if index < len(pt_text) else ''
        if not source:
            if index in fallback and tokens_match(original, fallback[index]):
                catalog[index] = fallback[index]
                report['no_ps2_text_used_fallback'].append(index)
            else:
                report['no_ps2_text'].append(index)
            continue
        text = adapt(original, source, names)
        if tokens_match(original, text):
            catalog[index] = text
            if english[index] != xbox[index]:
                report['english_differs_adapted'].append(index)
        elif index in fallback and tokens_match(original, fallback[index]):
            catalog[index] = fallback[index]
            report['token_conflict_used_fallback'].append(index)
        else:
            report['token_conflict_kept_english'].append(index)
    return catalog, dict(report)


def write_header(path: Path, catalog: dict[int, str]) -> None:
    lines = ['#pragma once', '#include <map>', '#include <cstdint>', '',
             '// Catalogo privado gerado por tools/ps2_ptbr_import.py a partir da ISO PS2 PT-BR.',
             '// Nao distribuir: textos derivados dos dados do jogo e do patch de traducao.',
             'namespace cod_ptbr {', 'inline const std::map<uint32_t,const char*> translations{']
    for index in sorted(catalog):
        # JSON sem escape ASCII: UTF-8 cru, escapes \n \" \\ validos em C++.
        lines.append(f'    {{{index},{json.dumps(catalog[index], ensure_ascii=False)}}},')
    lines += ['};', '}', '']
    path.write_text('\n'.join(lines), encoding='utf-8', newline='\n')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--xbox-dar', type=Path, default=Path('assets/data/strdataue.dar'))
    parser.add_argument('--out', type=Path, default=Path('assets/ptbr_strings.private.h'))
    parser.add_argument('--fallback', type=Path)
    parser.add_argument('--report', type=Path, default=Path('.local/ptbr-ps2/report.json'))
    args = parser.parse_args()

    xbox_raw = args.xbox_dar.read_bytes()
    xbox = xbox_strings(xbox_raw)
    english, ptbr, iso_info = ps2_tables(args.iso)
    fallback = read_catalog(args.fallback.read_text(encoding='utf-8')) if args.fallback and args.fallback.exists() else {}
    catalog, details = build_catalog(xbox, english, ptbr, fallback)
    originals = {i: s.decode('latin1') for i, s in enumerate(xbox)}
    result = audit(originals, catalog)
    result.update(details=details, iso=iso_info, xbox_sha256=hashlib.sha256(xbox_raw).hexdigest(),
                  ps2_english_count=len(english), ps2_ptbr_count=len(ptbr))
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    if result['failures']:
        print(f"Falhas: {len(result['failures'])}; header nao gravado. Veja {args.report}")
        return 1
    write_header(args.out, catalog)
    print(f"PT-BR PS2: {len(catalog)}/{result['nonempty_ids']} IDs; "
          + '; '.join(f'{k}={len(v)}' for k, v in details.items())
          + f"; {result['storage_bytes']}/{result['storage_limit']} bytes")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
