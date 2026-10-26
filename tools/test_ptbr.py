from pathlib import Path
import re
import struct
import unittest
import json

try:
    from ptbr_catalog import audit, glyph_supported, read_catalog, read_strings, supported_glyphs
except ModuleNotFoundError:
    from tools.ptbr_catalog import audit, glyph_supported, read_catalog, read_strings, supported_glyphs

ROOT = Path(__file__).resolve().parents[1]
PRIVATE_CATALOG = ROOT / 'assets/ptbr_strings.private.h'
PUBLIC_HEADER = ROOT / 'src/common/ptbr_strings.h'


class TranslationContract(unittest.TestCase):
    def test_local_catalog_covers_every_nonempty_table_entry(self):
        if not PRIVATE_CATALOG.exists():
            self.assertIn('translations{}', PUBLIC_HEADER.read_text().replace(' ', ''))
            return
        originals = read_strings((ROOT / 'assets/data/strdataue.dar').read_bytes())
        catalog = read_catalog(PRIVATE_CATALOG.read_text(encoding='utf-8'))
        report = audit(originals, catalog)
        self.assertEqual(report['missing_ids'], [], 'Tabela local ainda incompleta')
        self.assertEqual(report['failures'], [])
        self.assertTrue(report['table_complete'])

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
        extra = supported_glyphs(read_strings(data))
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
            # A cor tem oito digitos. Letras hexadecimais do texto seguinte nao
            # pertencem ao comando (por exemplo, Banco ou Cadeira).
            self.assertEqual(re.findall(r'\^C[0-9A-Fa-f]{8}|\^c', original),
                             re.findall(r'\^C[0-9A-Fa-f]{8}|\^c', translated), index)
            translated_text = json.loads('"' + translated + '"')
            self.assertTrue(all(glyph_supported(c, extra) for c in translated_text), index)


class CatalogAuditTests(unittest.TestCase):
    def dar(self, strings):
        data = bytearray(64 + 16 + len(strings) * 4)
        struct.pack_into('<I', data, 40, 32)
        struct.pack_into('<I', data, 68, len(strings))
        for index, text in enumerate(strings):
            struct.pack_into('<I', data, 80 + index * 4, len(data) - 64)
            data.extend(text + b'\0')
        return bytes(data)

    def test_reads_real_table_shape_and_mixed_bytes(self):
        strings = read_strings(self.dar([b'Menu', b'', b'Brand\xae']))
        self.assertEqual(strings, {0: 'Menu', 1: '', 2: 'Brand\xae'})

    def test_rejects_truncated_table_bad_offset_and_unterminated_string(self):
        valid = self.dar([b'Menu'])
        cases = [valid[:43], valid[:70], valid[:82], valid[:-1]]
        invalid = bytearray(valid)
        struct.pack_into('<I', invalid, 80, 1)
        cases.append(bytes(invalid))
        for data in cases:
            with self.subTest(size=len(data)):
                with self.assertRaises(ValueError):
                    read_strings(data)

    def test_cpp_literal_roundtrip_and_duplicate_rejection(self):
        text = 'Linha 1\n"Linha 2" \\teste'
        encoded = json.dumps(text)
        self.assertEqual(read_catalog('{12,' + encoded + '}'), {12: text})
        with self.assertRaisesRegex(ValueError, 'duplicado'):
            read_catalog('{12,"A"},{12,"B"}')

    def test_coverage_separates_preserved_names_and_empty_ids(self):
        report = audit({0: 'Menu', 1: 'Hector', 2: '', 3: 'Options'}, {0: 'Menu', 1: 'Hector'})
        self.assertEqual(report['nonempty_ids'], 3)
        self.assertEqual(report['preserved_ids'], [0, 1])
        self.assertEqual(report['missing_ids'], [3])
        self.assertFalse(report['table_complete'])

    def test_tokens_capacity_and_font_are_production_checks(self):
        original = {0: '^<KEY_A> %s %d ^C80404080Name^c'}
        good = {0: '^<KEY_A> %s %d ^C80404080Nome^c'}
        self.assertTrue(audit(original, good)['table_complete'])
        for text in ['^<KEY_B> %s %d ^C80404080Nome^c',
                     '^<KEY_A> %d %s ^C80404080Nome^c',
                     '^<KEY_A> %s %d ^C80404081Nome^c',
                     '^<KEY_A> %s %d ^C80404080A\0B^c',
                     # Fora do Windows-1252 e ausente da tabela original: sem glifo.
                     '^<KEY_A> %s %d ^C80404080Op\u0103oes^c']:
            self.assertTrue(audit(original, {0: text})['failures'])
        self.assertTrue(audit({0: 'A'}, {0: 'A' * 1048576})['failures'])

    def test_windows1252_accents_and_table_glyphs_are_supported(self):
        # O atlas Xbox cobre 0x21-0xFF do Windows-1252; ...' vem da propria tabela.
        self.assertTrue(audit({0: 'Options'}, {0: 'Op\u00e7\u00f5es \u00e3\u00ea\u00e1'})['table_complete'])
        self.assertTrue(audit({0: 'Wait\u2026'}, {0: 'Espere\u2026'})['table_complete'])
        fullwidth = 'Warp\uff1a'.encode('utf-8').decode('latin1')
        self.assertTrue(audit({0: fullwidth}, {0: 'Portal\uff1a'})['table_complete'])
        self.assertTrue(audit({0: 'Warp'}, {0: 'Portal\uff1a'})['failures'])
        self.assertTrue(audit({0: 'Warp'}, {0: 'A\tB'})['failures'])

    def test_named_tag_may_move_around_positional_value(self):
        self.assertTrue(audit({0: '%s ^<FMLR_NAME>'},
                              {0: '^<FMLR_NAME> produziu %s'})['table_complete'])

    def test_memory_card_substitutions_are_not_translated(self):
        self.assertTrue(audit({0: '[port] [size]'}, {0: '[port] [size]'})['table_complete'])
        self.assertTrue(audit({0: '[port] [size]'}, {0: '[entrada] [tamanho]'})['failures'])

    def test_color_consumes_eight_hex_digits_not_following_title_letters(self):
        self.assertTrue(audit({0: '^C80404080Stool^c'},
                              {0: '^C80404080Banco^c'})['table_complete'])
        self.assertTrue(audit({0: '^C80404080Stool^c'},
                              {0: '^C80404081Banco^c'})['failures'])


if __name__ == '__main__':
    unittest.main()
