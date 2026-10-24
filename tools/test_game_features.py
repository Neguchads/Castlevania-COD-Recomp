import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class GameFeatureSignatures(unittest.TestCase):
    def test_health_setter_matches_xbox_usa(self):
        image = (ROOT / "assets/default.xbe").read_bytes()
        expected = {
            0x326113: "8d b4 82 70 29 00 00 01 2e 79 06 c7 06 00 00 00 00",
            0x325F80: "8b 04 85 e8 14 b4 00 c3",
            0x3237A0: "55 8b ec b8 50 a3 b2 00 5d c3",
            0x3119A0: "56 33 f6 e8 98 27 d2 ff",
            0x2E7030: "53 8b 5c 24 08 55 56 57 bf 9c 8d 54",
            0x327A00: "8b 54 24 04 56 8b 74 24 0c 57 56 33 c9",
            0x2E6970: "b8 f4 34 b2 00 c3",
        }
        for offset, text in expected.items():
            signature = bytes.fromhex(text)
            with self.subTest(offset=hex(offset)):
                self.assertEqual(image[offset:offset+len(signature)], signature)

if __name__ == "__main__":
    unittest.main()
