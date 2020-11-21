# Test for SplineSetJoin Expand Stroke logic

import fontforge
font = fontforge.font()
glyph = font.createChar(0x30)
pen = glyph.glyphPen()
pen.moveTo((0, 0))
pen.lineTo((1, 0))
pen.lineTo((0, 0))
pen.endPath()
glyph.stroke('circular', 1, 'round')
