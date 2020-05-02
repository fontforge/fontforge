import fontforge

font = fontforge.font()
glyph = font.createChar(0x41)

# Any data in the foreground
pen = glyph.glyphPen()
pen.moveTo((0,0))
pen.lineTo((1,1))
pen.lineTo((1,0))
pen.closePath()
del pen

# Switch to BG
glyph.activeLayer = 0
glyph.background.is_quadratic = False
pen = glyph.glyphPen()
pen.moveTo((1,1))
pen.closePath()
del pen # crash if (auto)destroyed after font.close()...

font.generate('a.ttf', flags=('PfEd-background'))
font.close()

font = fontforge.open('a.ttf')
glyph = font[0x41]

assert len(glyph.foreground) == 1, 'Foreground contains a contour'
assert len(glyph.background) == 1, 'Background contains a contour'

fgc = fontforge.contour(True)
fgc.closed = True
fgc[:] = ((0,0), (1,1), (1,0))
# contour comparator is inexact, see PyFFContour_docompare / SSsCompare
assert glyph.foreground[0] == fgc, 'Foreground contour matches expected contour'
assert list(glyph.foreground[0]) == list(fgc), 'Foreground contour points matches expected contour points'

bgc = fontforge.contour(False)
bgc.closed = True
bgc[:] = ((1,1),)
assert glyph.background[0] == bgc, 'Background contour matches expected contour'
assert list(glyph.background[0]) == list(bgc), 'Foreground contour points matches expected contour points'

font.close()
