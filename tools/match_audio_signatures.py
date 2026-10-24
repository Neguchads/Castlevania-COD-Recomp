"""Compara assinaturas públicas com funções reais; apenas produz evidência."""
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]


def match():
    image = (ROOT / 'assets/default.xbe').read_bytes()
    base = struct.unpack_from('<I', image, 0x104)[0]
    count, pointer = struct.unpack_from('<II', image, 0x11c)
    sections = [struct.unpack_from('<5I', image, offset)
                for offset in range(pointer-base, pointer-base+count*56, 56)]

    def read(address, size):
        for _, virtual, length, raw, raw_length in sections:
            if virtual <= address and address+size <= virtual+min(length, raw_length):
                return image[raw+address-virtual:raw+address-virtual+size]
        return b''

    mapping = (ROOT/'generated/game/x86_func_mapping.cpp').read_text()
    starts = sorted(set(int(n, 16) for n in re.findall(r'\{0x([0-9A-Fa-f]+)u, sub_', mapping)))
    candidates = {address: read(address, 1024) for address in starts if 0x3E0000 <= address < 0x3FD000}
    report = []
    for source in sorted((ROOT/'.local/reference').glob('dsound-*.inl')):
        text = re.sub(r'/\*.*?\*/', '', source.read_text(), flags=re.S)
        text = re.sub(r'//[^\n]*', '', text)
        for block in re.finditer(r'OOVPA_SIG_HEADER\w*\s*\(\s*(\w+),.*?OOVPA_SIG_MATCH\((.*?)\n\);', text, re.S):
            name, body = block.groups()
            pairs = [(int(a, 16), int(b, 16)) for a, b in re.findall(r'\{\s*0x([0-9A-Fa-f]+),\s*0x([0-9A-Fa-f]+)\s*\}', body)]
            if len(pairs) < 7:
                continue
            refs = [(int(a, 16), b) for a, b in re.findall(r'XREF_ENTRY\(\s*0x([0-9A-Fa-f]+),\s*XREF_(\w+)\)', body)]
            hits = []
            for address, data in candidates.items():
                if all(offset < len(data) and data[offset] == value for offset, value in pairs):
                    targets = {}
                    for offset, target in refs:
                        if offset+4 <= len(data) and offset > 0 and data[offset-1] in (0xE8, 0xE9):
                            targets[target] = address+offset+4+struct.unpack_from('<i', data, offset)[0]
                    hits.append(dict(address=address, calls=targets))
            if hits:
                report.append(dict(name=name, source=source.name, bytes=len(pairs), hits=hits))
    (ROOT/'docs/audio-signature-matches.json').write_text(json.dumps(report, indent=2))
    unique = {}
    for row in report:
        if len(row['hits']) == 1:
            unique.setdefault(row['name'], set()).add(row['hits'][0]['address'])
    for name, values in sorted(unique.items()):
        if len(values) == 1:
            print(f'{name} = 0x{next(iter(values)):X}')


if __name__ == '__main__':
    match()
