# test some ligatures and fractions close to U0000. Avoid high values such as
# UFxxx since the Unicode chart may add new unicode values later.

import sys, fontforge

print("Get Table Array Totals.")
lt = fontforge.ucLigChartGetCnt();
vt = fontforge.ucVulChartGetCnt();
ot = fontforge.ucOFracChartGetCnt();
ft = fontforge.ucFracChartGetCnt();
if ( lt < 50 or lt > 2000 or vt < 10 or vt > 2000 or ot < 10 or vt > 2000 ):
  raise ValueError("Expected totals approximately closer to 500, 50, 20.")
if ( vt + ot != ft ):
  raise ValueError("Expected totals to add up.")

print("Test valid (and invalid) ligatures.")
l1 = fontforge.ucLigChartGetLoc(306)
l2 = fontforge.ucLigChartGetLoc(0x133)
l3 = fontforge.ucLigChartGetLoc(0x153)
l4 = fontforge.ucLigChartGetLoc(0x4b5)
l5 = fontforge.ucLigChartGetLoc(0x135)
if ( l1 != 0 or l2 != 1 or l3 != 3 or l4 != 7 or l5 != -1 ):
  raise ValueError("Expected Ligature table array 'n' values of 0,1,3,7,-1.")

print("Return some ligatures from built-in table.")
l1 = fontforge.ucLigChartGetNxt(0)
l2 = fontforge.ucLigChartGetNxt(1)
l3 = fontforge.ucLigChartGetNxt(3)
l4 = fontforge.ucLigChartGetNxt(7)
l5 = fontforge.ucLigChartGetNxt(lt) # table starts at 0, not 1 (last value is at [lt-1])
if ( l1 != 306 or l2 != 307 or l3 != 339 or l4 != 1205 or l5 != -1 ):
  raise ValueError("Expected Ligatures values from table of: 306,307,339,1205,-1.")

print("Test valid (and invalid) vulgar fractions.")
v1 = fontforge.ucVulChartGetLoc(0xbd)
v2 = fontforge.ucVulChartGetLoc(0x132)
if ( v1 != 1 or v2 != -1 ):
  raise ValueError("Expected Vulgar Fraction table array 'n' values of: 1,-1.")

print("Verify uint32 code works.")
o1 = fontforge.ucOFracChartGetNxt(ot-1)
if ( o1 <= 69000 ):
  raise ValueError("Expected last Other Fraction of 0x10e7e or larger.")

print("Test boolean found==1, not_found==0.")
b1 = fontforge.IsLigature(306)
b2 = fontforge.IsLigature(0x135)
b3 = fontforge.IsVulgarFraction(0xbd)
b4 = fontforge.IsVulgarFraction(0x132)
b5 = fontforge.IsFraction(0xbd)
b6 = fontforge.IsFraction(0x132)
if ( b1 != 1 or b2 != 0 or b3 != 1 or b4 != 0 or b5 != 1 or b6 != 0 ):
  raise ValueError("Expected boolean values of 1,0,1,0,1,0.")

print("All Tests done and valid.")
