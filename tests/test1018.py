#Needs: fonts/Ambrosia.sfd

#Test adding anchors and non linear transform of anchors
import sys, fontforge

ambrosia = sys.argv[1]
font = fontforge.open(ambrosia)

glyph = font["a"]
glyph.addAnchorPoint("top", "base", 176, 580)
glyph.addAnchorPoint("bottom", "base", 176, -20)

glyph = glyph.nltransform("200+(x-200)*abs(y-300)/300", "y^2")

assert glyph.anchorPoints[0][2] == 174.4, f"{glyph.anchorPoints[0][2]}"
assert glyph.anchorPoints[0][3] == 400, f"{glyph.anchorPoints[0][3]}"
assert glyph.anchorPoints[1][2] == 177.6, f"{glyph.anchorPoints[1][2]}"
assert glyph.anchorPoints[1][3] == 32767, f"{glyph.anchorPoints[1][3]}"
