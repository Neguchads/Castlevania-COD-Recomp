import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest
from extract_xiso import extract


class ExtractionTests(unittest.TestCase):
    def image(self, root, name=b'default.xbe', sector=34):
        data = bytearray(35 * 2048)
        data[65536:65556] = b'MICROSOFT*XBOX*MEDIA'
        struct.pack_into('<II', data, 65556, 33, 64)
        struct.pack_into('<HHIIBB', data, 33 * 2048, 0, 0, sector, 4, 0, len(name))
        data[33 * 2048 + 14:33 * 2048 + 14 + len(name)] = name
        data[34 * 2048:34 * 2048 + 4] = b'XBEH'
        source = root / 'fixture.iso'
        source.write_bytes(data)
        (root / 'docs').mkdir()
        return source

    def test_extract_and_verify(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            extract(self.image(root), root / 'assets')
            self.assertEqual((root / 'assets/default.xbe').read_bytes(), b'XBEH')
            report = json.loads((root / 'docs/extraction-manifest.json').read_text())
            self.assertEqual(report[0]['sha256'], hashlib.sha256(b'XBEH').hexdigest())

    def test_reject_path_escape(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaisesRegex(ValueError, 'Nome inseguro'):
                extract(self.image(root, b'../outside'), root / 'assets')

    def test_reject_out_of_image(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with self.assertRaisesRegex(ValueError, 'fora da imagem'):
                extract(self.image(root, sector=999), root / 'assets')


if __name__ == '__main__':
    unittest.main()
