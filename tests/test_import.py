# Import a simple hardcoded SVG file into a glyph and check that the glyph
# outline matches the original path.

import os
import tempfile

import fontforge

font = fontforge.font()
font.em = 1000
ascent = font.ascent

# The SVG y axis points down, so the font triangle (100,100) (500,100) (300,700)
# is written with y mirrored around the font ascent.
expected = [(100, 100), (500, 100), (300, 700)]
svg_points = [(x, ascent - y) for x, y in expected]
svg_text = (
    '<?xml version="1.0" standalone="no"?>\n'
    '<svg xmlns="http://www.w3.org/2000/svg" version="1.1">\n'
    '  <path d="M{} {} L{} {} L{} {} Z"/>\n'
    '</svg>\n'
).format(*[c for point in svg_points for c in point])


def glyph_points(glyph):
    return sorted(
        (round(point.x), round(point.y))
        for contour in glyph.foreground
        for point in contour
    )


glyph_a = font.createChar(ord("A"), "A")
glyph_a.width = 600

glyph_b = font.createChar(ord("B"), "B")
glyph_b.width = 600

with tempfile.TemporaryDirectory() as temp_dir:
    svg_path = os.path.join(temp_dir, "glyph-A.svg")
    with open(svg_path, "w", encoding="utf-8") as svg_file:
        svg_file.write(svg_text)

    # Import the SVG file into glyph A
    glyph_a.importOutlines(svg_path)

    # Import the SVG text as a blob into glyph B (not supported yet)
    glyph_b.importOutlines(bytearray(svg_text, "utf-8"), type="svg")

if len(glyph_a.foreground) != 1:
    raise ValueError("Expected exactly one contour in glyph A, got {}".format(
        len(glyph_a.foreground)))

if glyph_points(glyph_a) != sorted(expected):
    raise ValueError("Imported glyph A points {} differ from expected {}".format(
        glyph_points(glyph_a), sorted(expected)))

# if len(glyph_b.foreground) != 1:
#     raise ValueError("Expected exactly one contour in glyph B, got {}".format(
#         len(glyph_b.foreground)))
#
# if glyph_points(glyph_b) != sorted(expected):
#     raise ValueError("Imported glyph B points {} differ from expected {}".format(
#         glyph_points(glyph_b), sorted(expected)))

font.close()
