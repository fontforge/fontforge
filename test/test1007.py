#!/usr/local/bin/fontforge
#Needs: fonts/ayn+meem.init.svg

# At one point splinestroke failed if the first spline on a contour had a length of 0

import fontforge;

font=fontforge.font();
a = font.createChar(65);
a.importOutlines("fonts/ayn+meem.init.svg");
