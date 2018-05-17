# test some libuninameslist functions (if the library is included).

import sys, fontforge

ver = fontforge.UnicodeNamesListVersion()
Print("Libuninameslist version is :",ver)



# test Names2 functions (if we have libuninameslist version>=10.9)
cnt = fontforge.UnicodeNames2GetCntFromLib()
print("Libuninameslist internal table has ",cnt," Names2.")
if ( cnt < 25 )
  print("Test Names2 functions. Must be libuninameslist 10.9 or newer.")
  raise ValueError("Expected 25 or more values that have Names2.")

# none of these are -1 if library loaded
u0 = fontforge.UnicodeNames2NxtUniFromLib(0)
u1 = fontforge.UnicodeNames2NxtUniFromLib(1)
u2 = fontforge.UnicodeNames2NxtUniFromLib(2)
print("Sample of internal table has ",u0,", ",u1,", ",u2," as the first 3 unicode values.")
if ( u0 < 0 || u1 < 1 || u2 < 2 )
  raise ValueError("these values may be different, but expected 418,419,1801 for ver10.9.")

# We know these Names2 exist, but these 'tX' values may change if more errors found
t0 = fontforge.UnicodeNames2GetNxtFromLib(0x01A2)
t1 = fontforge.UnicodeNames2GetNxtFromLib(0xFEFF)
t2 = fontforge.UnicodeNames2GetNxtFromLib(118981)
print("Internal table[",t0,"]=0x01A2, table[",t1,"]=0xFEFF, table[",t2,"]=118981 as expected Names2 values.")
if ( t0 < 0 || t1 < 0 || t2 < 0 )
  raise ValueError("These Names2 must exist, so we expected these to return valid table locations.")

s0 = fontforge.UnicodeNames2FrmTabFromLib(3)
print("Example unicode from table[3]=",s0)
s1 = fontforge.UnicodeNames2FromLib(65)
s2 = fontforge.UnicodeNames2FromLib(0x709)
print("There should be no Names2 for character 65, the value returned is=",s1)
print("There should be a Names2 for character 0x709, the value returned is=",s2)
if ( Strlen(s0)<3 || Strlen(s2)<3 )
  raise ValueError("Expected to return a valid Names2 string.")

print("All Tests done and valid.")

