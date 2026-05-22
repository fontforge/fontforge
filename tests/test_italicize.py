#Needs: fonts/Ambrosia.sfd
import sys, fontforge

ambrosia = sys.argv[1]
font = fontforge.open(ambrosia)

font.italicize(lc_condense=(0.9, 0.9, 0.9, 0.9))
font.close()
