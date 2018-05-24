# test some ligatures and fractions close to U0000. Avoid high values such as
# UFxxx since the Unicode chart may add new unicode values later.

import sys, fontforge

print("Get Table Array Totals.")
lt = fontforge.ucLigChartGetCnt();
vt = fontforge.ucVulChartGetCnt();
ot = fontforge.ucOFracChartGetCnt();
ft = fontforge.ucFracChartGetCnt();
if lt < 50 or lt > 2000 or vt < 10 or vt > 2000 or ot < 10 or vt > 2000:
  raise ValueError("Expected totals approximately closer to 500, 50, 20.")
if vt + ot != ft:
  raise ValueError("Expected totals to add up.")

print("Test valid (and invalid) ligatures.")
l1 = fontforge.ucLigChartGetLoc(306)
l2 = fontforge.ucLigChartGetLoc(0x133)
l3 = fontforge.ucLigChartGetLoc(0x153)
l4 = fontforge.ucLigChartGetLoc(0x4b5)
l5 = fontforge.ucLigChartGetLoc(0x135)
if l1 != 0 or l2 != 1 or l3 != 3 or l4 != 7 or l5 != -1:
  raise ValueError("Expected Ligature table array 'n' values of 0,1,3,7,-1.")

print("Return some ligatures from built-in table.")
l1 = fontforge.ucLigChartGetNxt(0)
l2 = fontforge.ucLigChartGetNxt(1)
l3 = fontforge.ucLigChartGetNxt(3)
l4 = fontforge.ucLigChartGetNxt(7)
l5 = fontforge.ucLigChartGetNxt(lt) # table starts at 0, not 1 (last value is at [lt-1])
if l1 != 306 or l2 != 307 or l3 != 339 or l4 != 1205 or l5 != -1:
  raise ValueError("Expected Ligatures values from table of: 306,307,339,1205,-1.")

print("Test valid (and invalid) vulgar fractions.")
v1 = fontforge.ucVulChartGetLoc(0xbd)
v2 = fontforge.ucVulChartGetLoc(0x132)
if v1 != 1 or v2 != -1:
  raise ValueError("Expected Vulgar Fraction table array 'n' values of: 1,-1.")

print("Verify uint32 code works.")
o1 = fontforge.ucOFracChartGetNxt(ot-1)
if o1 <= 69000:
  raise ValueError("Expected last Other Fraction of 0x10e7e or larger.")

print("Test boolean found==1, not_found==0.")
b1 = fontforge.IsLigature(306)
b2 = fontforge.IsLigature(0x135)
b3 = fontforge.IsVulgarFraction(0xbd)
b4 = fontforge.IsVulgarFraction(0x132)
b5 = fontforge.IsFraction(0xbd)
b6 = fontforge.IsFraction(0x132)
if b1 != 1 or b2 != 0 or b3 != 1 or b4 != 0 or b5 != 1 or b6 != 0:
  raise ValueError("Expected boolean values of 1,0,1,0,1,0.")

print("Test valid (and invalid) vulgar fraction alt expansions.")
e1 = fontforge.ucVulChartGetLoc(0xbc)
e2 = fontforge.ucVulChartGetLoc(0x2152)
e3 = fontforge.ucVulChartGetAltCnt(e1)
e4 = fontforge.ucVulChartGetAltCnt(e2)
e5 = fontforge.ucVulChartGetAltCnt(1000)
e6 = fontforge.ucVulChartGetAltVal(e1,0)
e7 = fontforge.ucVulChartGetAltVal(e1,2)
e8 = fontforge.ucVulChartGetAltVal(e2,2)
e9 = fontforge.ucVulChartGetAltVal(e2,3)
if e1 < 0 or e2 < 0 or e3 != 3 or e4 != 4 or e5 != -1 or e6 != 49 or e7 != 52 or e8 != 49 or e9 != 48:
  print("Returned %s %s %s %s %s %s %s %s %s" % (e1,e2,e3,e4,e5,e6,e7,e8,e9))
  raise ValueError("Expected alt_expansion_count results of {0,5,3,4,-1,49,52,49,48}.")

print("Test ligature alt expansions.")
f1 = fontforge.ucLigChartGetLoc(0xfdfa)
f2 = fontforge.ucLigChartGetLoc(0xfdfb)
f3 = fontforge.ucLigChartGetAltCnt(f1)
f4 = fontforge.ucLigChartGetAltCnt(f2)
f5 = fontforge.ucLigChartGetAltVal(f1,0)
f6 = fontforge.ucLigChartGetAltVal(f1,1)
f7 = fontforge.ucLigChartGetAltVal(f1,2)
f8 = fontforge.ucLigChartGetAltVal(f1,3)
f9 = fontforge.ucLigChartGetAltVal(f1,20)
f10 = fontforge.ucLigChartGetAltVal(f2,3)
f11 = fontforge.ucLigChartGetAltVal(f2,4)
f12 = fontforge.ucLigChartUGetAltCnt(0x153)
f13 = fontforge.ucLigChartUGetAltCnt(0x4d5)
f14 = fontforge.ucLigChartUGetAltVal(0x4d5,0)
f15 = fontforge.ucLigChartUGetAltVal(0x4d5,1)
f16 = fontforge.ucLigChartUGetAltCnt(0x00011176)
f17 = fontforge.ucLigChartUGetAltVal(0x00011176,0)
if f1 < 0 or f2 < 0 or f3 != 18 or f4 != 7 or f5 != 0x635 or f6 != 0x644 or f7 != 0x649 or f8 != 0x20 or f9 != -1 or f10 != 0x62c or f11 != 0x644 or f12 != 2 or f13 != 2 or f14 != 0x430 or f15 != 0x435 or f16 != 0 or f17 != -1:
  print("Returned %s %s %s %s %s %s %s %s %s" % (f1,f2,f3,f4,f5,f6,f7,f8,f9))
  print("Returned %s %s %s %s %s %s %s %s" % (f10,f11,f12,f13,f14,f15,f16,f17))
  raise ValueError("Expected alt_expansion_count results of {496,497,18,7,1589,1604,1609,32,-1,1580,1604,2,2,1072,1077,0,-1}.")

print("All Tests done and valid.")
