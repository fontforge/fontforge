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

#### Invalid Layer Array ####
font2=fontforge.open(sys.argv[1])
layers = font2["B"].layers 
font2.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    new_l = layers[0].dup()
    # The execution shall not reach this line
    assert(False)
except ValueError:
    exception_occured = True

assert(exception_occured)

#### Invalid Layer References Dictionary ####
font2=fontforge.open(sys.argv[1])
l_refs = font2["B"].layerrefs
font2.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    refs = l_refs["Fore"]
    # The execution shall not reach this line
    assert(False)
except ValueError:
    exception_occured = True

assert(exception_occured)
