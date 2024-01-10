import sys, fontforge

font = fontforge.font()

glyph = font.createChar(0x20)

# In a previous version, this would give an error:
#   TypeError: 'float' object cannot be interpreted as an integer
glyph.right_side_bearing += 0
glyph.left_side_bearing += 0
