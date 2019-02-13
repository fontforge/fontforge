# Point type conversion tests

import fontforge

co = fontforge.splineCorner
cu = fontforge.splineCurve
tn = fontforge.splineTangent
hv = fontforge.splineHVCurve

ff = fontforge.font()

g = ff.createMappedChar('a')

c = fontforge.contour()
c[:] = [(0,0,True,hv),(0,0,False),(5,-1,False),(5,0,True,hv),(5,1,False),(5,3,False),(4,4,True,hv),(3,5,False),(0,3,False),(0,2,True,hv)]
c.closed = True

l = fontforge.layer()
l += c

g.setLayer(l,1,('select_none','hvcurve'))
cc = g.layers[1][0]
for (i,t) in [(0,hv),(3,hv),(6,hv),(9,hv)]:
    if cc[i].type != t:
        raise ValueError("Type of point " + str(i) + " should be " + str(t) + " rather than " + str(cc[i].type))

g.setLayer(l,1,('select_all','hvcurve','force'))
cc = g.layers[1][0]
for (i,t) in [(1,False),(2,True),(4,True),(5,False),(7,False),(8,False)]:
    if (cc[i] == c[i]) != t:
        print(cc[i],c[i])
        raise ValueError("Coordinates of point " + str(i) + " should have " + ("stayed the same" if t else "changed"))

c[9].type = tn
g.setLayer(l,1,('select_all','hvcurve','downgrade'))
cc = g.layers[1][0]
for (i,t) in [(0,co),(3,hv),(6,cu),(9,tn)]:
    if cc[i].type != t:
        raise ValueError("Type of point " + str(i) + " should be " + str(t) + " rather than " + str(cc[i].type))

c[9].type = cu
c[3].type = tn
c[6].type = co
g.setLayer(l,1,('select_incompat','hvcurve','by_geom'))
cc = g.layers[1][0]
for (i,t) in [(0,co),(3,hv),(6,co),(9,tn)]:
    if cc[i].type != t:
        raise ValueError("Type of point " + str(i) + " should be " + str(t) + " rather than " + str(cc[i].type))

c[3].type = cu
c[9].type = cu
g.setLayer(l,1,('select_smooth','hvcurve','by_geom'))
cc = g.layers[1][0]
for (i,t) in [(0,co),(3,hv),(6,co),(9,tn)]:
    if cc[i].type != t:
        raise ValueError("Type of point " + str(i) + " should be " + str(t) + " rather than " + str(cc[i].type))

c[3].type = cu
c[9].type = cu
g.setLayer(l,1,('select_smooth','by_geom'))
cc = g.layers[1][0]
for (i,t) in [(0,co),(3,cu),(6,co),(9,tn)]:
    if cc[i].type != t:
        raise ValueError("Type of point " + str(i) + " should be " + str(t) + " rather than " + str(cc[i].type))

c[0].type = c[3].type = c[6].type = c[9].type = co
g.setLayer(l,1,('select_all','by_geom'))
cc = g.layers[1][0]
for (i,t) in [(0,co),(3,cu),(6,cu),(9,tn)]:
    if cc[i].type != t:
        raise ValueError("Type of point " + str(i) + " should be " + str(t) + " rather than " + str(cc[i].type))

c[3].type = hv
try:
    g.setLayer(l,1,('select_all','check'))
except:
    raise ValueError("setLayer with 'check' failed when all types compatible with geometry")

c[3].type = tn
try:
    g.setLayer(l,1,('select_all','check'))
except:
    pass
else:
    raise ValueError("setLayer with 'check' did not raise error for incompatible type")

try:
    g.setLayer(l,1,('select_all','select_none','by_geom'))
except:
    pass
else:
    raise ValueError("setLayer did not raise error for incompatible select flags")

try:
    g.setLayer(l,1,('select_all','check','by_geom'))
except:
    pass
else:
    raise ValueError("setLayer did not raise error for incompatible conversion mode flags")
