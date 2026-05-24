#Needs: fonts/Ambrosia.sfd
import sys, fontforge

ambrosia = sys.argv[1]
font = fontforge.open(ambrosia)

# Make sure that this doesn't segfault
font.italicize(lc_condense=(0.9, 0.9, 0.9, 0.9))

# Test the new xheight_percent keyword argument
font.italicize(xheight_percent=0.80)

font.close()
