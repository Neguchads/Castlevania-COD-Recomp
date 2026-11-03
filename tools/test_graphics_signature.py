from pathlib import Path
import struct
import tomllib
import unittest

ROOT = Path(__file__).resolve().parents[1]


class GraphicsSignatureTests(unittest.TestCase):
    def test_palette_binding_matches_xbe(self):
        data = (ROOT/'assets/default.xbe').read_bytes()
        config = tomllib.loads((ROOT/'config/game.toml').read_text())
        address = config['symbols']['D3DDevice_SetPalette']
        base = struct.unpack_from('<I', data, 0x104)[0]
        count, pointer = struct.unpack_from('<II', data, 0x11c)
        for offset in range(pointer-base, pointer-base+count*56, 56):
            _, virtual, size, raw, raw_size = struct.unpack_from('<5I', data, offset)
            if virtual <= address < virtual+min(size, raw_size):
                body = data[raw+address-virtual:raw+address-virtual+141]
                break
        else:
            self.fail('Funcao de paleta fora das secoes do XBE')
        # Lê e escreve CDevice_Palettes[stage] (+0xF98), dois argumentos stdcall.
        self.assertEqual(body[13:20], bytes.fromhex('8b84be980f0000'))
        self.assertEqual(body[69:76], bytes.fromhex('899cbe980f0000'))
        self.assertEqual(body[138:141], bytes.fromhex('c20800'))


if __name__ == '__main__':
    unittest.main()
