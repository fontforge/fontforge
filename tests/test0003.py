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

xmin, ymin, xmax, ymax = font[2].boundingBox()

assert (xmin, xmax) == font[2].xBoundsAtY(ymin, ymax)
assert font[2].xBoundsAtY(ymax+1) == None

assert (ymin, ymax) == font[2].yBoundsAtX(xmin, xmax)
assert font[2].yBoundsAtX(xmax+1) == None
