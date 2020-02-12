#Needs: fonts/BoxesAndBounds.py

import sys, fontforge

font = fontforge.open(sys.argv[1])

expected = (63.0, 57.0, 315.0, 309.0)
assert font[0].boundingBox() == expected
assert font[1].boundingBox(layer=1) == expected
nullbb = (0.0, 0.0, 0.0, 0.0)
assert font[1].boundingBox(layer=0) == nullbb

expected = (63.0, 57.0, 315.0, 711.0)
assert font[2].boundingBox() == expected
expected = (63.0, 459.0, 315.0, 711.0)
assert font[2].boundingBox(layer=2) == expected
