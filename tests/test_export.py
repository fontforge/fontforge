# Create a font with one glyph and another glyph referencing it, export the
# referencing glyph to SVG, and read the SVG text back.

import os
import tempfile

import fontforge

font = fontforge.font()
font.em = 1000

base = font.createChar(ord("A"), "A")
base.width = 600
pen = base.glyphPen()
pen.moveTo((100, 100))
pen.lineTo((500, 100))
pen.lineTo((300, 700))
pen.closePath()
pen = None

ref = font.createChar(ord("B"), "B")
ref.width = 600
ref.addReference("A")

if len(ref.references) != 1:
    raise ValueError("Expected exactly one reference in glyph B")

# Export a glyph with a reference to SVG and check that the file is created and
# contains a <path>.
with tempfile.TemporaryDirectory() as temp_dir:
    svg_path = os.path.join(temp_dir, "glyph-B.svg")

    ref.export(svg_path)

    if not os.path.exists(svg_path):
        raise ValueError("SVG file was not created")

    with open(svg_path, "r", encoding="utf-8") as svg_file:
        svg_text = svg_file.read()
        print(svg_text)

    if "<path" not in svg_text:
        raise ValueError("Exported SVG does not contain <path")

font.close()
