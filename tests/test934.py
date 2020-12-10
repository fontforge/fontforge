#Needs: fonts/SFDBitmapParsing.sfd
#Test for parsing a malformed SFD

import fontforge
import os
import sys

font = fontforge.open(sys.argv[1])
names = sorted((g.glyphname for g in font.glyphs()))
assert(names == ["A", "B", "C", "D", "E"])

font.save('test934.cleaned.sfd')
