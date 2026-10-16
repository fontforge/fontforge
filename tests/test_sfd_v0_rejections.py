# A version 0 SFD with these records must remain rejected. The parser uses
# the file version during loading even if it normalizes the loaded font later.
import os
import tempfile

import fontforge

records = ("ScriptLang: 1\n1\n'latn' 1 'dflt'",
           "TableOrder: 'GSUB' 1\n'liga'")

with tempfile.TemporaryDirectory() as tmp:
    path = os.path.join(tmp, "old.sfd")
    for record in records:
        with open(path, "w", newline="\n") as sfd:
            sfd.write("SplineFontDB: 0\nFontName: A\n" + record +
                      "\nBeginChars: 256 0\nEndChars\nEndSplineFont\n")
        try:
            font = fontforge.open(path)
        except OSError:
            pass
        else:
            font.close()
            raise AssertionError("v0 " + record.split(":")[0] + " must be rejected")
