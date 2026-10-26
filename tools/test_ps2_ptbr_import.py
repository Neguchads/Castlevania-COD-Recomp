import struct
import tempfile
import unittest
from pathlib import Path

try:
    import ps2_ptbr_import as imp
except ModuleNotFoundError:
    from tools import ps2_ptbr_import as imp


def table(strings):
    """Recurso de texto: u32 count em +4, offsets u32 em +16, strings com NUL."""
    head = bytearray(16 + 4 * len(strings))
    struct.pack_into('<I', head, 4, len(strings))
    body = bytearray()
    for index, text in enumerate(strings):
        struct.pack_into('<I', head, 16 + 4 * index, len(head) + len(body))
        body += text + b'\0'
    return bytes(head + body)


def dar(entries):
    """DAR com offsets relativos ao registro de cada entrada."""
    head = bytearray(0x20 + 16 * len(entries))
    head[:4] = imp.DAR_MAGIC
    struct.pack_into('<I', head, 0x10, len(entries))
    struct.pack_into('<I', head, 0x14, 0x20)
    body = bytearray()
    for index, (entry_id, data) in enumerate(entries):
        record = 0x20 + 16 * index
        struct.pack_into('<IIII', head, record, entry_id, 0, len(head) + len(body) - record, len(data))
        body += data
    blob = head + body
    struct.pack_into('<I', blob, 8, len(blob))
    return bytes(blob)


def padded(blob):
    return blob + b'\0' * (-len(blob) % imp.SECTOR)


def iso_with(name, content):
    """ISO9660 minima: PVD no setor 16, raiz no setor 18, arquivo no setor 20."""
    image = bytearray(20 * imp.SECTOR) + padded(content)
    pvd = 16 * imp.SECTOR
    image[pvd:pvd + 6] = b'\x01CD001'
    root = bytearray(34)
    root[0] = 34
    struct.pack_into('<I', root, 2, 18)
    struct.pack_into('<I', root, 10, imp.SECTOR)
    image[pvd + 156:pvd + 190] = root
    ident = (name + ';1').encode()
    record = bytearray(33 + len(ident) + (len(ident) + 1) % 2)
    record[0] = len(record)
    struct.pack_into('<I', record, 2, 20)
    struct.pack_into('<I', record, 10, len(content))
    record[32] = len(ident)
    record[33:33 + len(ident)] = ident
    image[18 * imp.SECTOR:18 * imp.SECTOR + len(record)] = record
    return bytes(image)


class ImportTests(unittest.TestCase):
    def test_entry_offset_is_relative_to_its_record(self):
        blob = dar([(7, b'AAAA'), (9, b'BB')])
        self.assertEqual(imp.dar_entries(blob), {7: b'AAAA', 9: b'BB'})

    def test_reads_tables_from_filedata_inside_iso(self):
        english = dar([(imp.EN_TABLE, table([b'Map', b'Wait^<SANTEN>']))])
        portuguese = dar([(imp.PTBR_TABLE, table([b'Mapa', 'Espere^<SANTEN>'.encode()]))])
        filedata = b'\0' * imp.SECTOR + padded(english) + padded(portuguese)
        with tempfile.TemporaryDirectory() as tmp:
            iso = Path(tmp) / 'test.iso'
            iso.write_bytes(iso_with('FILEDATA.BIN', filedata))
            en, pt, info = imp.ps2_tables(iso)
        self.assertEqual(en, [b'Map', b'Wait^<SANTEN>'])
        self.assertEqual(pt[0], b'Mapa')
        self.assertEqual(info['distinct_copies'], {str(imp.EN_TABLE): 1, str(imp.PTBR_TABLE): 1})

    def test_conflicting_copies_of_a_table_are_rejected(self):
        a = dar([(imp.PTBR_TABLE, table([b'Mapa'])), (imp.EN_TABLE, table([b'Map']))])
        b = dar([(imp.PTBR_TABLE, table([b'Outro']))])
        filedata = b'\0' * imp.SECTOR + padded(a) + padded(b)
        with tempfile.TemporaryDirectory() as tmp:
            iso = Path(tmp) / 'test.iso'
            iso.write_bytes(iso_with('FILEDATA.BIN', filedata))
            with self.assertRaisesRegex(ValueError, 'versoes distintas'):
                imp.ps2_tables(iso)

    def test_copy_with_emptied_subtitles_is_merged_with_full_copy(self):
        full = table([b'Menu', 'Legenda é'.encode()])
        emptied = table([b'Menu', b''])
        self.assertEqual(imp.merge_copies([imp.table_strings(emptied), imp.table_strings(full)], 1),
                         [b'Menu', 'Legenda é'.encode()])
        with self.assertRaisesRegex(ValueError, 'contagens'):
            imp.merge_copies([[b'A'], [b'A', b'B']], 1)

    def test_adapt_converts_pal_tags_and_colored_titles(self):
        self.assertEqual(imp.adapt('Wait...', 'Espere^<SANTEN>', {}), 'Espere…')
        self.assertEqual(imp.adapt('the Belmont name', 'o nome ^<Bel>', {}), 'o nome Belmont')
        self.assertEqual(imp.adapt('^<Bel> clan', 'clã ^<Bel>', {}), 'clã ^<Bel>')
        xbox = '^C80404080Stump Stool^c\nA rough seat.'
        self.assertEqual(imp.adapt(xbox, 'Um assento rústico.', {'Stump Stool': 'Banquinho'}),
                         '^C80404080Banquinho^c\nUm assento rústico.')
        self.assertEqual(imp.adapt('Warp: go', '^C80404080Portal:^c ir', {}), 'Portal: ir')

    def test_adapt_restores_name_tags_written_out_by_translator(self):
        self.assertEqual(imp.adapt('Are you not ^<Trevor> ^<Bel>...', 'Você não é Trevor Belmont...', {}),
                         'Você não é ^<Trevor> ^<Bel>...')
        self.assertEqual(imp.adapt('^<Trevor> unlocked! "@^<TREVOR>"', 'Trevor liberado! "@^<TREVOR>"', {}),
                         '^<Trevor> liberado! "@^<TREVOR>"')
        self.assertEqual(imp.adapt('Hello', 'Olá Belmont', {}), 'Olá Belmont')

    def test_symbol_only_and_missing_entries(self):
        xbox = [b'^<DOLLAR>1', b'Learn by:', b'-']
        catalog, report = imp.build_catalog(xbox, xbox, [b'1 $', b'', b''], {1: 'Aprenda:'})
        self.assertEqual(catalog, {0: '^<DOLLAR>1', 1: 'Aprenda:', 2: '-'})
        self.assertEqual(report['symbols_only_kept'], [0, 2])
        self.assertEqual(report['no_ps2_text_used_fallback'], [1])

    def test_build_catalog_uses_same_id_and_reports_conflicts(self):
        xbox = [b'Map', b'', b'Stump Stool', b'^C80404080Stump Stool^c\nSeat.', b'^<KEY_A> go', b'%s left']
        english = [b'Map', b'', b'Stump Stool', b'Seat.', b'^<KEY_A> go', b'%s left']
        ptbr = ['Mapa', 'x', 'Banquinho', 'Assento.', 'vai', '%s restantes'] + ['extra PAL']
        catalog, report = imp.build_catalog(xbox, english, [s.encode() for s in ptbr], {4: '^<KEY_A> ir'})
        self.assertEqual(catalog[0], 'Mapa')
        self.assertNotIn(1, catalog)  # ID vazio no Xbox continua vazio
        self.assertEqual(catalog[3], '^C80404080Banquinho^c\nAssento.')
        self.assertEqual(catalog[4], '^<KEY_A> ir')
        self.assertEqual(report['token_conflict_used_fallback'], [4])
        self.assertEqual(report['english_differs_adapted'], [3])
        catalog, report = imp.build_catalog(xbox, english, [s.encode() for s in ptbr], {})
        self.assertNotIn(4, catalog)
        self.assertEqual(report['token_conflict_kept_english'], [4])

    def test_header_roundtrip_keeps_utf8_and_escapes(self):
        try:
            from ptbr_catalog import read_catalog
        except ModuleNotFoundError:
            from tools.ptbr_catalog import read_catalog
        catalog = {3: 'Coração\n"máx" \\ +', 1: 'Nível…'}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'h.h'
            imp.write_header(path, catalog)
            text = path.read_text(encoding='utf-8')
        self.assertIn('Coração', text)
        self.assertEqual(read_catalog(text), catalog)


if __name__ == '__main__':
    unittest.main()
