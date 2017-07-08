#Needs: fonts/feta20.pfb

# feta is a font for music symbols, so reencode
# to ucs4 means no encoding slots are used; but
# you can still select by the symbol names.

import sys, fontforge
font = fontforge.open( sys.argv[1] )
font.selection.all()
if ( font.isWorthOutputting() != True ):
    raise ValueError("Wrong return from fontforge.fonts after selection.all")

font.selection.none()
if ( font.isWorthOutputting() != False ):
    raise ValueError("Wrong return from fontforge.fonts after selection.none")

font.reencode("ucs4")
try:
    font[256]
except TypeError:
    # expect this!
    pass
else:
    raise ValueError("Unexpected Error")

font.selection.select(256)
if ( font.isWorthOutputting() != False ):
    raise ValueError("Wrong return from fontforge.fonts after selecting unused slot")

font.selection.all()
if ( font.isWorthOutputting() != True ):
    raise ValueError("Wrong return from fontforge.fonts after 2nd selection.all")

font.selection.none()
if ( font.isWorthOutputting() != False ):
    raise ValueError("Wrong return from fontforge.fonts after 2nd selection.none")

font.selection.select(32)
if ( font.isWorthOutputting() != False ):
    raise ValueError("Wrong return from fontforge.fonts after 2nd unused slot")

font.selection.select("accidentals.sharp")
if ( font.isWorthOutputting() != True ):
    raise ValueError("Wrong return from fontforge.fonts after used named slot")

font.selection.none()
if ( font.isWorthOutputting() != False ):
    raise ValueError("Wrong return from fontforge.fonts after 3rd selection.nome")

font.selection.select(33)
if ( font.isWorthOutputting() != False ):
    raise ValueError("Wrong return from fontforge.fonts after 3rd unused slot")

print("font.isWorthOutputting tests done success")
