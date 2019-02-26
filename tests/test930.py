# Point, contour, and layer fidelity tests

import copy
import fontforge as f

# point

# Embedded tuple
p = f.point((2.3,2.4))
if p.x != 2.3 or p.y != 2.4 or not p.on_curve or p.selected:
    raise ValueError("Bad point values" )

p = f.point((2.3,2.4,False))
if p.x != 2.3 or p.y != 2.4 or p.on_curve or p.selected:
    raise ValueError("Bad point values" )

p = f.point((2.3,2.4,False,True))
if p.x != 2.3 or p.y != 2.4 or p.on_curve or not p.selected:
    raise ValueError("Bad point values" )

# Embedded List 
p = f.point([2.3,2.4])
if p.x != 2.3 or p.y != 2.4 or not p.on_curve or p.selected:
    raise ValueError("Bad point values" )

p = f.point([2.3,2.4,False])
if p.x != 2.3 or p.y != 2.4 or p.on_curve or p.selected:
    raise ValueError("Bad point values" )

p = f.point([2.3,2.4,False,True])
if p.x != 2.3 or p.y != 2.4 or p.on_curve or not p.selected:
    raise ValueError("Bad point values" )

# Unembedded
p = f.point(2.3,2.4)
if p.x != 2.3 or p.y != 2.4 or not p.on_curve or p.selected:
    raise ValueError("Bad point values" )

p = f.point(2.3,2.4,False)
if p.x != 2.3 or p.y != 2.4 or p.on_curve or p.selected:
    raise ValueError("Bad point values" )

p = f.point(2.3,2.4,False,True)
if p.x != 2.3 or p.y != 2.4 or p.on_curve or not p.selected:
    raise ValueError("Bad point values" )

# dup
p.name = 'test'
p2 = p.dup()
if p2.x != 2.3 or p2.y != 2.4 or p2.on_curve or not p2.selected or p2.name != 'test':
    raise ValueError("Bad point values" )

if p2 != (2.3,2.4):
    raise ValueError("Bad point comparison" )

if p2 != p:
    raise ValueError("Bad point comparison" )

# contour 

ptl = [(0,0),(1,1,False),(2,2,False),(3,3),(4,4),(5,5,False),(6,6,False),(7,7),(8,8,False),(9,0,False)]
ptl2 = [(4.2,4.2),(4.5,4.5),(4.7,4.7)]

c = f.contour()
if len(c) != 0 or c.is_quadratic:
    raise ValueError("Bad contour values" )

c = f.contour(True) # Set quadratic 
if len(c) != 0 or not c.is_quadratic:
    raise ValueError("Bad contour values" )

c.is_quadratic = False
if c.is_quadratic:
    raise ValueError("Bad contour values" )

for pt in ptl:
    c.insertPoint(pt)
c.closed = True;
if len(c) != 10:
    raise ValueError("Bad contour length" )

c2 = f.contour()
c2[:] = ptl
c2.closed = True;
if len(c2) != 10:
    raise ValueError("Bad contour length" )

# Needs corrected glyphcomp.c spline comparison code
#if c2 != c:
if list(c2) != list(c):
    raise ValueError("Bad contour comparison" )

cp = c2[0]
cp.x = cp.y = -1

if c2 == c:
    raise ValueError("Bad contour comparison" )

c2[0] = c[0]
if list(c2) != list(c):
    raise ValueError("Bad contour comparison" )

del c2[3]
if c2[3].x != 4 or len(c2) != 9:
    raise ValueError("Bad contour value after delete" )

c2.insertPoint(ptl[3], 2)
if list(c2) != list(c):
    raise ValueError("Bad contour value after delete" )

del c2[3]
c2[3:3] = ptl[3:4]
if list(c2) != list(c):
    raise ValueError("Bad contour value after delete" )

del c2[3]
c2.insertPoint(c[3], 2)
if list(c2) != list(c):
    raise ValueError("Bad contour value after delete" )

del c2[3]
c2[3:3] = c[3:4]
if list(c2) != list(c):
    raise ValueError("Bad contour value after delete" )

c[:] = ptl
c += ptl2
c2[:] = ptl + ptl2
if list(c2) != list(c):
    raise ValueError("Bad contour value after concat" )

c2[:] = ptl
c3 = f.contour()
c3[:] = ptl2
if list(c) != list(c2 + c3):
    raise ValueError("Bad contour value from concat" )

c[:] = ptl
c2[:] = ptl

try:
    c2.makeFirst(11)
except TypeError:
    pass
else:
    raise ValueError("MakeFirst allowed out of bounds")

try:
    c2.makeFirst(1)
except TypeError:
    pass
else:
    raise ValueError("MakeFirst allowed on off-curve point")

c2.makeFirst(3)
if list(c2) != list(c[3:] + c[:3]):
    raise ValueError("MakeFirst corrupted list")

c2.makeFirst(len(c)-3)
if list(c2) != list(c):
    raise ValueError("MakeFirst corrupted list")

tmpptl = copy.copy(ptl)
tmpptl[3:7] = ptl2
c[:] = tmpptl
c2[:] = ptl
c2[3:7] = ptl2
if list(c2) != list(c):
    raise ValueError("MakeFirst corrupted list")

tmpptl = copy.copy(ptl)
tmpptl[8:5:-1] = ptl2
c[:] = tmpptl
c2[:] = ptl
c2[8:5:-1] = ptl2
if list(c2) != list(c):
    raise ValueError("MakeFirst corrupted list")

i = iter(c)
j = iter(c2)
k = 0
while True:
    try:
        nc = next(i)
        nc2 = next(j)
    except:
        break
    k += 1
    if nc != nc2:
        raise ValueError("Corrupted contour iterator")

if k != 10:
    raise ValueError("Corrupted contour iterator")

pn = f.point(.5,.5)

if not c2[3] in c or pn in c2 or not (1,1) in c2 or (.5,.5) in c2:
    raise ValueError("Bad contour contains")

l = f.layer()
if len(l) != 0 or l.is_quadratic:
    raise ValueError("Bad layer values")

l2 = f.layer(True)
if len(l2) != 0 or not l2.is_quadratic:
    raise ValueError("Bad layer values")

#c[:] = ptl
try:
    l2 += c
except:
    pass
else:
    raise TypeError("Layer accepted contour of wrong type")

l += c
if len(l) != 1 or list(l[0]) != list(c):
    raise TypeError("Wrong layer values")

c2[:] = ptl[3:6]
l += c2
if len(l) != 2 or list(l[1]) != list(c2):
    raise TypeError("Wrong layer values")

del l[0]
if len(l) != 1 or list(l[0]) != list(c2):
    raise TypeError("Wrong layer values")

l2 = f.layer()
c3[:] = ptl2
l2 += c3

l3 = l + l2
if len(l3) != 2:
    raise TypeError("Bad layer concat")
