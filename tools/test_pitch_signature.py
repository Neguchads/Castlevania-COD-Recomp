"""Confere a identificação SetPitch na imagem real antes do build."""
from pathlib import Path
import struct
import unittest

ROOT=Path(__file__).resolve().parents[1]

class PitchSignatureTests(unittest.TestCase):
    def test_signature_and_call_chain(self):
        data=(ROOT/'assets/default.xbe').read_bytes()
        base=struct.unpack_from('<I',data,0x104)[0]
        count,pointer=struct.unpack_from('<II',data,0x11c)
        def read(address,size):
            for offset in range(pointer-base,pointer-base+count*56,56):
                _,virtual,length,raw,raw_length=struct.unpack_from('<5I',data,offset)
                if virtual<=address and address+size<=virtual+min(length,raw_length):
                    return data[raw+address-virtual:raw+address-virtual+size]
            raise ValueError('Endereco fora das secoes')
        wrapper=read(0x3E252A,28)
        self.assertEqual(wrapper[4:8],bytes.fromhex('ff742408'))
        self.assertEqual(wrapper[10:13],bytes.fromhex('83c0e4'))
        self.assertEqual(wrapper[25:28],bytes.fromhex('c20800'))
        self.assertEqual(0x3E252A+25+struct.unpack_from('<i',wrapper,21)[0],0x3E23B6)
        voice=read(0x3E18B6,25)
        self.assertEqual(voice[11:14],bytes.fromhex('895118'))
        self.assertEqual(voice[22:25],bytes.fromhex('c20800'))
        # Chamadas e escritas distinguem rolloff de Doppler no XDK 5849.
        for wrapper_address, buffer_address, voice_address, field in (
            (0x3E30E6, 0x3E2F9E, 0x3E2C5B, 0x48),
            (0x3E30C2, 0x3E2F48, 0x3E2C28, 0x4C),
        ):
            wrapper = read(wrapper_address, 36)
            self.assertEqual(wrapper[28], 0xE8)
            self.assertEqual(wrapper_address+33+struct.unpack_from('<i', wrapper, 29)[0], buffer_address)
            self.assertEqual(read(voice_address+17, 3), bytes((0x89, 0x50, field)))
        play = read(0x3E2586, 36)
        self.assertEqual(play[28], 0xE8)
        self.assertEqual(0x3E2586+33+struct.unpack_from('<i',play,29)[0],0x3E1B03)
        self.assertEqual(play[33:36],bytes.fromhex('c21000'))

if __name__=='__main__':
    unittest.main()
