# One CTest source for old SFD conversion and version-0 record rejection.
import os
import tempfile

import fontforge

KERN_V0 = """SplineFontDB: 0
FontName: A
Encoding: iso8859_1
BeginChars: 256 1
StartChar: A
Encoding: 65 65
Width: 1000
Kerns: 65 -346
EndChar
EndChars
EndSplineFont
"""
ANCHOR_V1 = """SplineFontDB: 1.0
FontName: A
AnchorClass2: "A" ""
BeginChars: 256 0
EndChars
EndSplineFont
"""
REJECT_V0 = "SplineFontDB: 0\nFontName: A\n{record}\nBeginChars: 256 0\nEndChars\nEndSplineFont\n"


def open_sfd(directory, text):
    path = os.path.join(directory, "old.sfd")
    with open(path, "w", newline="\n") as sfd:
        sfd.write(text)
    return fontforge.open(path)


def check_kern(directory):
    font = open_sfd(directory, KERN_V0)
    try:
        assert len(font.gpos_lookups) == 1, "v0 Kerns: lookup missing"
        assert font.getLookupInfo(font.gpos_lookups[0])[0] == "gpos_pair"
        pairs = [pos for pos in font["A"].getPosSub("*") if pos[1] == "Pair"]
        assert [(pos[2], pos[5]) for pos in pairs] == [("A", -346)], pairs
    finally:
        font.close()


def check_anchor(directory):
    font = open_sfd(directory, ANCHOR_V1)
    try:
        assert len(font.gpos_lookups) == 1, "v1 AnchorClass2: lookup missing"
        assert font.getLookupInfo(font.gpos_lookups[0])[0] == "gpos_mark2base"
    finally:
        font.close()


def check_rejections(directory):
    for record in ("ScriptLang: 1\n1\n'latn' 1 'dflt'",
                   "TableOrder: 'GSUB' 1\n'liga'"):
        try:
            font = open_sfd(directory, REJECT_V0.format(record=record))
        except OSError:
            continue
        font.close()
        raise AssertionError("v0 " + record.split(":")[0] + " must be rejected")


def main():
    with tempfile.TemporaryDirectory() as directory:
        check_kern(directory)
        check_anchor(directory)
        check_rejections(directory)


if __name__ == "__main__":
    main()
