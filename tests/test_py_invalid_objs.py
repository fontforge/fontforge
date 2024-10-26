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
except RuntimeError:
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
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Layer References Dictionary ####
font3 = fontforge.open(sys.argv[1])
l_refs = font3["C"].layerrefs
font3.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    refs = l_refs["Fore"]
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Math Kerning Data ####
font4 = fontforge.open(sys.argv[1])
mk_data = font4["D"].mathKern
font4.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    bl = mk_data.bottomLeft
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Glyph Object ####
font5 = fontforge.open(sys.argv[1])
glyph_E = font5["E"]
glyph_F = font5["F"]
font5.close()

# Try to access object getter after the font was deleted
exception_occured = False
try:
    tti = glyph_E.ttinstrs
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

# Try to access object function after the font was deleted
exception_occured = False
try:
    glyph_F.correctDirection()
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Layer Info Array ####
font6 = fontforge.open(sys.argv[1])
layers = font6.layers
font6.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    layers.add("new-layer", False)
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Layer Info ####
font7 = fontforge.open(sys.argv[1])
fore_info = font7.layers["Fore"]
font7.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    is_q = fore_info.is_quadratic
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Private Dictionary ####
font8 = fontforge.open(sys.argv[1])
private = font8.private
font8.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    private["BlueValues"] = None
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Selection ####
font9 = fontforge.open(sys.argv[1])
sel_ref = font9.selection
font9.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    sel_ref.select("A", "C", "Z")
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Cvt table ####
font10 = fontforge.open(sys.argv[1])
cvt = font10.cvt
font10.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    cvt += [10, 12]
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Math table ####
font11 = fontforge.open(sys.argv[1])
math = font11.math
font11.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    has = math.exists()
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)

#### Invalid Math Device table ####
font12 = fontforge.open(sys.argv[1])
devtable = font12.math.AxisHeightDeviceTable
font12.close()

# Try to access object after the font was deleted
exception_occured = False
try:
    devtable[10] = 2
    # The execution shall not reach this line
    assert(False)
except RuntimeError:
    exception_occured = True

assert(exception_occured)
