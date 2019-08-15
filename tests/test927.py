#Needs: fonts/CantarellMin.woff2

import fontforge
import sys

sfd = fontforge.open(sys.argv[1])
sfd.generate("CantarellMin.woff2")
sfd.close()

woff = fontforge.open("CantarellMin.woff2")
woff.save("CantarellMin.sfd")
woff.close()
