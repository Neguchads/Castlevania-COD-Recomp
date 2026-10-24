from pathlib import Path
import re
import struct
import unittest

ROOT = Path(__file__).resolve().parents[1]
PRIVATE_CATALOG = ROOT / 'assets/ptbr_strings.private.h'
PUBLIC_HEADER = ROOT / 'src/common/ptbr_strings.h'


class TranslationContract(unittest.TestCase):
    def test_private_catalog_matches_game_or_public_fallback_is_empty(self):
        if not PRIVATE_CATALOG.exists():
            source = PUBLIC_HEADER.read_text(encoding='utf-8')
            entries = re.findall(r'\{(\d+),"((?:[^"\\]|\\.)*)"\}', source)
            self.assertEqual(entries, [])
            self.assertIn('translations{}', source.replace(' ', ''))
            return

        data = (ROOT / 'assets/data/strdataue.dar').read_bytes()
        resource = 32 + struct.unpack_from('<I', data, 40)[0]
        count = struct.unpack_from('<I', data, resource + 4)[0]
        source = PRIVATE_CATALOG.read_text(encoding='utf-8')
        entries = re.findall(r'\{(\d+),"((?:[^"\\]|\\.)*)"\}', source)
        self.assertGreater(len(entries), 400)
        self.assertEqual(len(entries), len({entry[0] for entry in entries}))
        for raw_id, translated in entries:
            index = int(raw_id)
            self.assertLess(index, count)
            offset = struct.unpack_from('<I', data, resource + 16 + index * 4)[0]
            start = resource + offset
            original = data[start:data.index(b'\0', start)].decode('latin1')
            self.assertTrue(original, index)
            self.assertEqual(re.findall(r'\^<[^>]+>', original), re.findall(r'\^<[^>]+>', translated), index)
            self.assertEqual(re.findall(r'%(?:\d+)?[sdf]', original), re.findall(r'%(?:\d+)?[sdf]', translated), index)
            self.assertEqual(re.findall(r'\^[Cc][0-9A-Fa-f]*', original), re.findall(r'\^[Cc][0-9A-Fa-f]*', translated), index)
            self.assertTrue(translated.isascii(), index)


if __name__ == '__main__':
    unittest.main()
