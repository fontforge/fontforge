#Needs: fonts/Ambrosia.sfd

import sys, fontforge

#### Invalid Glyph Pen ####
font=fontforge.open(sys.argv[1])
pen = font["A"].glyphPen() 
font.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    pen.moveTo((100,100))
    pen.lineTo((100,200))
    pen = None
    # The execution shall not reach this line
    assert(False)
except ValueError:
    exception_occured = True

assert(exception_occured)
