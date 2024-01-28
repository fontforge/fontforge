#Test deleting points from contour

import fontforge

cont = fontforge.contour()
cont.moveTo(0, 0).lineTo(10, 0).lineTo(20, 0).lineTo(30, 0)

c = cont.dup()
del c[1]
assert len(c) == 3
assert c[0].x == 0
assert c[1].x == 20
assert c[2].x == 30

c = cont.dup()
del c[1:3]
assert len(c) == 2
assert c[0].x == 0
assert c[1].x == 30
