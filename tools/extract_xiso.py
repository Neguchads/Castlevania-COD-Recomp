"""Extrai XDVDFS com limites de leitura e verifica cada arquivo por SHA-256."""
import hashlib
import json
from pathlib import Path
import struct
import sys


def extract(source, destination):
    source, destination = Path(source), Path(destination).resolve()
    total = source.stat().st_size
    manifest = []
    with source.open('rb') as image:
        def read(offset, size):
            if offset < 0 or size < 0 or offset + size > total:
                raise ValueError('Leitura fora da imagem')
            image.seek(offset)
            data = image.read(size)
            if len(data) != size:
                raise ValueError('Imagem truncada')
            return data

        base = next((b for b in (0, 0x18300000)
                     if read(b + 65536, 20) == b'MICROSOFT*XBOX*MEDIA'), None)
        if base is None:
            raise ValueError('Volume XDVDFS ausente')
        root_sector, root_size = struct.unpack('<II', read(base + 65536 + 20, 8))
        visited = set()

        def folder(sector, size, relative, depth=0):
            if depth > 32 or size > 16 * 1024 * 1024:
                raise ValueError('Diretorio fora dos limites')
            if (sector, size) in visited:
                raise ValueError('Diretorio repetido')
            visited.add((sector, size))
            if not size:
                return
            table = read(base + sector * 2048, size)
            todo, seen = [0], set()
            while todo:
                offset = todo.pop()
                if offset in seen:
                    raise ValueError('Ciclo na tabela')
                seen.add(offset)
                if offset + 14 > size:
                    raise ValueError('Entrada truncada')
                left, right, entry_sector, length, attr, count = struct.unpack_from('<HHIIBB', table, offset)
                if left == 65535 and right == 65535:
                    continue
                if offset + 14 + count > size:
                    raise ValueError('Nome truncado')
                name = table[offset + 14:offset + 14 + count].decode('ascii')
                if not name or name in ('.', '..') or any(c in name for c in '/\\:'):
                    raise ValueError('Nome inseguro')
                path = relative / name
                target = (destination / path).resolve()
                if not target.is_relative_to(destination):
                    raise ValueError('Destino fora da pasta')
                if attr & 16:
                    target.mkdir(parents=True, exist_ok=True)
                    folder(entry_sector, length, path, depth + 1)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True)
                    digest = hashlib.sha256()
                    with target.open('xb') as output:
                        for start in range(0, length, 1024 * 1024):
                            block = read(base + entry_sector * 2048 + start, min(1024 * 1024, length - start))
                            output.write(block)
                            digest.update(block)
                    with target.open('rb') as saved:
                        actual = hashlib.file_digest(saved, 'sha256').hexdigest()
                    if actual != digest.hexdigest():
                        raise ValueError(f'Falha de integridade: {path}')
                    manifest.append(dict(path=path.as_posix(), size=length, sha256=actual))
                todo.extend(child * 4 for child in (right, left) if child)

        folder(root_sector, root_size, Path())
    report = destination.parent / 'docs' / 'extraction-manifest.json'
    report.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    print(f'Verificados: {len(manifest)} arquivos, {sum(item["size"] for item in manifest)} bytes')


if __name__ == '__main__':
    extract(sys.argv[1], sys.argv[2])
