# test some libuninameslist functions (if the library is included).

import sys, fontforge

cnt = fontforge.UnicodeBlockCountFromLib()
if cnt < 1:
  print("No NamesList library - Nothing to test.")
  quit()

if cnt < 294:
  print("Pre-20170701 or older library - Nothing to test beyond here.")
  quit()

ver = fontforge.UnicodeNamesListVersion()
print("Libuninameslist version is : %s" % ver)

# test Names2 functions (if we have libuninameslist version>=10.9)
if cnt < 305:
  print("Pre-20180408 or older library - Nothing to test beyond here.")
  quit()

cnt = fontforge.UnicodeNames2GetCntFromLib()
print("Libuninameslist internal table has %s Names2." % cnt)
if cnt < 25:
  print("Test Names2 functions. Must be libuninameslist 10.9 or newer.")
  raise ValueError("Expected 25 or more values that have Names2.")

# none of these are -1 if library loaded
u0 = fontforge.UnicodeNames2NxtUniFromLib(0)
u1 = fontforge.UnicodeNames2NxtUniFromLib(1)
u2 = fontforge.UnicodeNames2NxtUniFromLib(2)
print("Sample of internal table has %s, %s, %s as the first 3 unicode values." % (u0, u1, u2))
if u0 < 0 or u1 < 1 or u2 < 2:
  raise ValueError("these values may be different, but expected 418,419,1801 for ver10.9.")

# We know these Names2 exist, but these 'tX' values may change if more errors found
t0 = fontforge.UnicodeNames2GetNxtFromLib(0x01A2)
t1 = fontforge.UnicodeNames2GetNxtFromLib(0xFEFF)
t2 = fontforge.UnicodeNames2GetNxtFromLib(118981)
print("Internal table[%s]=0x01A2, table[%s]=0xFEFF, table[%s]=118981 as expected Names2 values." % (t0, t1, t2))
if t0 < 0 or t1 < 0 or t2 < 0:
  raise ValueError("These Names2 must exist, so we expected these to return valid table locations.")

s0 = fontforge.UnicodeNames2FrmTabFromLib(3)
print("Example unicode from table[3]=%s" % s0)
s1 = fontforge.UnicodeNames2FromLib(65)
s2 = fontforge.UnicodeNames2FromLib(0x709)
print("There should be no Names2 for character 65, the value returned is=%s" % s1)
print("There should be a Names2 for character 0x709, the value returned is=%s" % s2)
if len(s0) < 3 or len(s2) < 3:
  raise ValueError("Expected to return a valid Names2 string.")

print("All Tests done and valid.")

