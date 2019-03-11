# Round-trip point selection tests.

import copy
import fontforge as ff
import pickle

c0 = [(293.0,48.0,1), (291.0,48.0,1), (271.0,32.0,1), (227.0,-2.0,0), (196.8,-10.0,0), (162.0,-10.0,1), (92.0,-10.0,0), (36.0,16.0,0), (36.0,98.0,1), (36.0,166.0,0), (105.0,219.0,0), (201.0,243.0,1), (287.0,264.0,1), (290.0,265.0,0), (293.0,269.0,0), (293.0,276.0,1), (293.0,389.0,0), (246.0,406.0,0), (212.0,406.0,1), (174.0,406.0,0), (132.0,395.0,0), (132.0,364.0,1), (132.0,353.0,0), (133.0,347.0,0), (134.0,344.0,1), (136.0,340.0,0), (137.0,333.0,0), (137.0,326.0,1), (137.0,313.0,0), (119.0,292.0,0), (90.0,292.0,1), (67.0,292.0,0), (55.0,304.0,0), (55.0,328.0,1), (55.0,385.0,0), (138.0,439.0,0), (220.0,439.0,1), (293.0,439.0,0), (371.0,412.0,0), (371.0,270.0,1), (371.0,123.0,1), (371.0,77.0,0), (372.0,38.0,0), (401.0,38.0,1), (413.7,38.0,0), (430.5,47.8,0), (438.0,54.0,1), (449.3,47.7,0), (453.3,39.3,0), (455.0,27.0,1), (434.3,7.0,0), (398.3,-10.0,0), (360.0,-10.0,1), (309.6,-10.0,0), (299.0,17.0,0)]

f = ff.font()

g = f.createMappedChar('a')

l = ff.layer()
c = ff.contour()

# Cubic 
c[:] = c0
c.closed = True
l += c

for (i,p) in enumerate(l[0]):
    if i % 5 == 0:
        p.selected = True

g.layers[1] = l
l = g.layers[1]

for (i,p) in enumerate(l[0]):
    if (i % 5 == 0 and not p.selected) or (i % 5 != 0 and p.selected):
        raise ValueError("Round trip of layer changed selection status")

for (i,p) in enumerate(l[0]):
    p.selected = True

g.layers[1] = l
l = g.layers[1]

for (i,p) in enumerate(l[0]):
    if not p.selected:
        print(i, p)
        raise ValueError("Round trip of layer changed selection status")

# Quadratic

f.is_quadratic = True
g.layers[1] = l
l = g.layers[1]

for (i,p) in enumerate(l[0]):
    p.selected = True

g.layers[1] = l
l = g.layers[1]

for (i,p) in enumerate(l[0]):
    if not p.selected:
        print(i, p)
        raise ValueError("Round trip of layer changed selection status")

for (i,p) in enumerate(l[0]):
    if i % 3 == 0:
        p.selected = True
    else:
        p.selected = False

g.layers[1] = l
l = g.layers[1]

for (i,p) in enumerate(l[0]):
    if (i % 3 == 0 and not p.selected) or (i % 3 != 0 and p.selected):
        raise ValueError("Round trip of layer changed selection status")

# Special attention to the tricky quadratic case

for (i,p) in enumerate(l[0]):
    p.selected = False

pl = l[0][-1]

if p.on_curve:
    raise ValueError("Trailing quadratic contour point should be off-curve")

pl.selected = True

g.layers[1] = l
l = g.layers[1]

lclen = len(l[0])

for (i,p) in enumerate(l[0]):
    if (i < (lclen-1) and p.selected) or (i == lclen-1 and not p.selected):
        raise ValueError("Round trip of layer changed selection status")

# Point Pickling 

p = ff.point(4.4, 5.5)
pps = pickle.dumps(p)
fpp = pickle.loads(pps)

if fpp.x != 4.4 or fpp.y != 5.5 or not fpp.on_curve or fpp.selected or fpp.interpolated:
    print(fpp, fpp.selected, fpp.interpolated)
    raise ValueError("Pickling lost a value")

p.on_curve = False
p.selected = True
pps = pickle.dumps(p)
fpp = pickle.loads(pps)

if fpp.x != 4.4 or fpp.y != 5.5 or fpp.on_curve or not fpp.selected or fpp.interpolated:
    print(fpp, fpp.selected, fpp.interpolated)
    raise ValueError("Pickling lost a value")

p.selected = False
p.interpolated = True
pps = pickle.dumps(p)
fpp = pickle.loads(pps)

if fpp.x != 4.4 or fpp.y != 5.5 or fpp.on_curve or fpp.selected or not fpp.interpolated:
    print(fpp, fpp.selected, fpp.interpolated)
    raise ValueError("Pickling lost a value")

