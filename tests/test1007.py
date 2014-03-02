#Needs: fonts/ayn+meem.init.svg

# At one point splinestroke failed if the first spline on a contour had a length of 0

import sys, fontforge

font=fontforge.font()
a = font.createChar(65)
a.importOutlines(sys.argv[1])
