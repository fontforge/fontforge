#Needs: fonts/InterpolateCompatibleCheck.sfd
#Needs: fonts/InterpolateCompatibleCheck2.sfd

import fontforge
import sys

f1 = fontforge.open(sys.argv[1])
f2 = fontforge.open(sys.argv[2])

f1["A"].layers["Fore"].interpolateNewLayer(f2["A"].layers["Fore"], 0.5, True)
fontforge.font().createInterpolatedGlyph(f1["B"], f2["B"], 0.5)

try:
    fontforge.font().createInterpolatedGlyph(f1["B"], f2["B"], 0.5, True)
    raise AssertionError
except OSError: # Unfortunately changing this now would break API. People are expecting OSError.
    pass

f3 = f1.interpolateFonts(0.5, sys.argv[2], 0, True)
f3g = set([g.glyphname for g in f3.glyphs()])
assert f3g == set(["A",  "Agrave", "space"])

f4 = f1.interpolateFonts(0.5, sys.argv[2], 0, False)
assert len(list(f4.glyphs())) == len(list(f1.glyphs()))
