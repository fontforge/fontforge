import sys, fontforge

font = fontforge.open(sys.argv[1])
glyphs = list(font.glyphs())
assert(glyphs[0].glyphname == ".notdef")
assert(glyphs[55].glyphname == "uni0F62.2.left")
assert(glyphs[1329].glyphname == "uni0F720F7E")
