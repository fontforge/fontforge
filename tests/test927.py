#Needs: fonts/Ambrosia.sfd

import fontforge
import sys

sfd = fontforge.open(sys.argv[1])
sfd.generate("Ambrosia.woff2")
sfd.close()

woff = fontforge.open("Ambrosia.woff2")
woff.save("Ambrosia.sfd")
woff.close()
