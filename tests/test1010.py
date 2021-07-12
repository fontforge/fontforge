# unicode tests

import sys, fontforge, unittest, re


class UnicodeTests(unittest.TestCase):
    def test_UnicodeNameFromLib(self):
        for i in [-1, 0, 0x110000, 0x1100001]:
            self.assertEqual(
                "", fontforge.UnicodeNameFromLib(i), f"{i} should have no name"
            )

        # Longest name as of Unicode 13.0.0
        self.assertEqual(
            "BOX DRAWINGS LIGHT DIAGONAL UPPER CENTRE TO MIDDLE RIGHT AND MIDDLE LEFT TO LOWER CENTRE",
            fontforge.UnicodeNameFromLib(0x1FBA9),
        )

        # All ASCII
        for i in range(0x110000):
            name = fontforge.UnicodeNameFromLib(i)
            self.assertTrue(name.isascii(), f"U+{i:04X} is all ascii")

    def test_UnicodeAnnotationFromLib(self):
        for i in [-1, 0x110000, 0x1100001]:
            self.assertEqual(
                "",
                fontforge.UnicodeAnnotationFromLib(i),
                f"{i} should have no annotation",
            )

        # Longest annotation as of Unicode 13.0.0
        annot = fontforge.UnicodeAnnotationFromLib(0x27)
        self.assertTrue(
            annot.startswith("= apostrophe-quote (1.0)"), "apostrophe has annotation"
        )

        # utf-8/unicode support
        annot = fontforge.UnicodeAnnotationFromLib(0x2052)
        self.assertIn("abzüglich", annot)

    def test_unicode_blocks(self):
        self.assertGreater(fontforge.UnicodeBlockCountFromLib(), 0)
        for i in range(fontforge.UnicodeBlockCountFromLib()):
            self.assertGreater(len(fontforge.UnicodeBlockNameFromLib(i)), 0)
            self.assertGreaterEqual(fontforge.UnicodeBlockStartFromLib(i), 0)
            self.assertGreaterEqual(
                fontforge.UnicodeBlockEndFromLib(i),
                fontforge.UnicodeBlockStartFromLib(i),
            )

    def test_UnicodeNamesListVersion(self):
        self.assertIsNotNone(
            re.match(
                r"NamesList-Version: \d+(?:\.\d+)+", fontforge.UnicodeNamesListVersion()
            )
        )

    def test_UnicodeNames2FromLib(self):
        for i in [-1, 0, 1, 0x110000, 0x1100001]:
            self.assertEqual(
                "",
                fontforge.UnicodeNames2FromLib(i),
                f"{i} should have no formal alias",
            )
        self.assertEqual(
            "WEIERSTRASS ELLIPTIC FUNCTION", fontforge.UnicodeNames2FromLib(0x2118)
        )
        self.assertEqual(
            "PRESENTATION FORM FOR VERTICAL RIGHT WHITE LENTICULAR BRACKET",
            fontforge.UnicodeNames2FromLib(0xFE18),
        )


if __name__ == "__main__":
    unittest.main()
