#Needs: fonts/Ambrosia.sfd

import sys, fontforge

font = fontforge.open(sys.argv[1])

# Set new SFNT value
font.appendSFNTName("English (US)", "SubFamily", "Dummy Val")
family = next(x for x in font.sfnt_names if x[0]=="English (US)" and x[1]=="SubFamily")
assert(family[2]=="Dummy Val")

# reset to default SFNT value
font.appendSFNTName("English (US)", "SubFamily", "Medium")
family = next(x for x in font.sfnt_names if x[0]=="English (US)" and x[1]=="SubFamily")
assert(family[2]=="Medium")
