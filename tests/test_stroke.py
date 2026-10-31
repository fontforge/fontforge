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

# Crash in FindMonoContaining() on aarch64
font = fontforge.font()
glyph = font.createChar(-1, 'repro')
pen = glyph.glyphPen()
# A circle of radius 125 (rotated 30 degrees) with a straight exit line.
pen.moveTo((-108.253, 62.5))
pen.curveTo((-142.771, 2.713), (-122.287, -73.735), (-62.5, -108.253))
pen.curveTo((-2.713, -142.771), (73.735, -122.287), (108.253, -62.5))
pen.curveTo((142.771, -2.713), (122.287, 73.735), (62.5, 108.253))
pen.curveTo((2.713, 142.771), (-73.735, 122.287), (-108.253, 62.5))
pen.lineTo((-45.753, 170.753))
pen.endPath()
glyph.stroke('circular', 101, 'round')   # SIGSEGV
