"""Audita o catalogo local sem modificar os arquivos extraidos do jogo."""

import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ENTRY = re.compile(r'\{(\d+),"((?:[^"\\]|\\.)*)"\}')
TOKEN = re.compile(r'\^<[^>]+>|\^C[0-9A-Fa-f]{8}|\^c|%(?:\d+)?[sdf]')
TOKEN_GROUPS = (re.compile(r'\^<[^>]+>'), re.compile(r'\^C[0-9A-Fa-f]{8}|\^c'),
                re.compile(r'%(?:\d+)?[sdf]'), re.compile(r'\[(?:port|size)\]'))
STORAGE_LIMIT = 1048576


def read_strings(data: bytes) -> dict[int, str]:
    """Le a primeira tabela DAR usada pelo getter de textos desta edicao."""
    if len(data) < 44:
        raise ValueError('DAR truncado: cabecalho')
    resource = 32 + struct.unpack_from('<I', data, 40)[0]
    if resource + 16 > len(data):
        raise ValueError('DAR truncado: recurso')
    count = struct.unpack_from('<I', data, resource + 4)[0]
    table_end = resource + 16 + count * 4
    if table_end > len(data):
        raise ValueError('DAR truncado: tabela de offsets')
    result = {}
    for index in range(count):
        offset = struct.unpack_from('<I', data, resource + 16 + index * 4)[0]
        start = resource + offset
        if start < table_end or start >= len(data):
            raise ValueError(f'Offset de texto invalido: ID {index}')
        end = data.find(b'\0', start)
        if end < 0:
            raise ValueError(f'Texto sem terminador: ID {index}')
        # A tabela mistura UTF-8 e bytes isolados (por exemplo, marca registrada).
        # Latin-1 mantem cada byte sem descartar dados; tokens sao sempre ASCII.
        result[index] = data[start:end].decode('latin1')
    return result


def read_catalog(source: str) -> dict[int, str]:
    result = {}
    for raw_id, encoded in ENTRY.findall(source):
        index = int(raw_id)
        if index in result:
            raise ValueError(f'ID duplicado: {index}')
        result[index] = json.loads('"' + encoded + '"')
    return result


def supported_glyphs(originals: dict[int, str]) -> set[str]:
    """Caracteres nao Windows-1252 que a propria tabela Xbox ja usa (UTF-8)."""
    seen = set()
    for text in originals.values():
        # read_strings devolve bytes como Latin-1; texto ja decodificado entra direto.
        try:
            seen.update(text.encode('latin1').decode('utf-8', 'ignore'))
        except UnicodeEncodeError:
            seen.update(text)
    return seen


def glyph_supported(char: str, extra: set[str]) -> bool:
    # O atlas da fonte Xbox segue Windows-1252 (0x21-0xFF, incluindo acentos);
    # a tabela PAL grava esses caracteres em UTF-8, como o Xbox faz com '...'.
    if char == '\n':
        return True
    if ord(char) < 0x20:
        return False
    try:
        char.encode('cp1252')
        return True
    except UnicodeEncodeError:
        return char in extra


def audit(originals: dict[int, str], catalog: dict[int, str]) -> dict:
    failures = []
    extra = supported_glyphs(originals)
    for index, text in catalog.items():
        original = originals.get(index)
        if not original:
            failures.append({'id': index, 'reason': 'ID ausente ou vazio no original'})
            continue
        if not text or '\0' in text or not all(glyph_supported(c, extra) for c in text):
            failures.append({'id': index, 'reason': 'Texto vazio, NUL ou glifo nao suportado'})
        # Tags nomeadas podem mudar de posicao na frase. A ordem dos parametros
        # posicionais e a sequencia de cores/botoes devem continuar intactas.
        if any(pattern.findall(original) != pattern.findall(text) for pattern in TOKEN_GROUPS):
            failures.append({'id': index, 'reason': 'Tokens ou ordem do mesmo tipo alterados'})
    nonempty = {index for index, text in originals.items() if text}
    missing = sorted(nonempty - catalog.keys())
    preserved = sorted(index for index in nonempty & catalog.keys()
                       if originals[index] == catalog[index])
    stored_bytes = sum(len(text.encode('utf-8')) + 1 for text in catalog.values())
    # O seletor de idioma compartilha a mesma area de memoria do catalogo.
    stored_bytes += len('Português (Brasil)'.encode('utf-8')) + 1
    if stored_bytes > STORAGE_LIMIT:
        failures.append({'reason': 'Catalogo excede armazenamento do runtime'})
    # Sinaliza revisao visual, sem confundir comprimento com largura de glifo.
    expanded = sorted(index for index in nonempty & catalog.keys()
                      if max(map(len, TOKEN.sub('', catalog[index]).splitlines()), default=0)
                      > max(map(len, TOKEN.sub('', originals[index]).splitlines()), default=0) * 1.35 + 8)
    return {
        'scope': 'IDs nao vazios da tabela fornecida; nao inclui texto em textura/video',
        'total_ids': len(originals), 'nonempty_ids': len(nonempty),
        'catalog_ids': len(catalog), 'translated_ids': len(nonempty & catalog.keys()) - len(preserved),
        'preserved_ids': preserved, 'missing_ids': missing,
        'storage_bytes': stored_bytes, 'storage_limit': STORAGE_LIMIT,
        'layout_review_ids': expanded, 'failures': failures,
        'table_complete': not missing and not failures,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('assets/data/strdataue.dar'))
    parser.add_argument('--catalog', type=Path, default=Path('assets/ptbr_strings.private.h'))
    parser.add_argument('--report', type=Path, default=Path('.local/ptbr-work/coverage.json'))
    parser.add_argument('--require-complete', action='store_true')
    args = parser.parse_args()
    raw = args.source.read_bytes()
    report = audit(read_strings(raw), read_catalog(args.catalog.read_text(encoding='utf-8')))
    report['source_sha256'] = hashlib.sha256(raw).hexdigest()
    report['catalog_sha256'] = hashlib.sha256(args.catalog.read_bytes()).hexdigest()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, ensure_ascii=True, indent=2) + '\n', encoding='utf-8')
    print(f"PT-BR: {report['catalog_ids']}/{report['nonempty_ids']} IDs; "
          f"{len(report['missing_ids'])} pendentes; {len(report['failures'])} erros; "
          f"{report['storage_bytes']}/{STORAGE_LIMIT} bytes.")
    return int(bool(report['failures']) or (args.require_complete and not report['table_complete']))


if __name__ == '__main__':
    raise SystemExit(main())
