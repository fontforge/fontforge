# Regression test for Debian bug #948876
# (use-after-free when MergeFonts copies glyph MATH data: the merged glyphs
#  shared vert_variants/horiz_variants/mathkern with the source font, and
#  the source font is freed right after the merge)

import os, tempfile, fontforge

def add_glyph(font, uni, name):
    g = font.createChar(uni)
    g.glyphname = name
    p = g.glyphPen()
    p.moveTo(0, 0)
    p.lineTo(100, 0)
    p.lineTo(100, 100)
    p.lineTo(0, 100)
    p.closePath()
    return g

def make_source(path, a_variants, a_italic, e_variants, e_italic):
    src = fontforge.font()
    add_glyph(src, ord('A'), "A")
    add_glyph(src, ord('E'), "E")
    add_glyph(src, ord('b'), "b")
    g = src[ord('A')]
    g.verticalVariants = a_variants
    g.verticalComponents = (("b", 1, 20, 20, 200),)
    g.verticalComponentItalicCorrection = a_italic
    g = src[ord('E')]
    g.verticalVariants = e_variants
    g.verticalComponents = (("b", 1, 20, 20, 200),)
    g.verticalComponentItalicCorrection = e_italic
    src.save(path)

with tempfile.TemporaryDirectory() as tmpdirname:
    # The MATH data must be copied into the merged font, not shared with the
    # (freed) source font.
    srcfile = os.path.join(tmpdirname, "mergefont_math_src.sfd")

    dst = fontforge.font()
    make_source(srcfile, "B.v C.v", 20, "X.v Y.v", 99)
    dst.mergeFonts(srcfile)
    make_source(srcfile, "E.v F.v", 42, "P.v Q.v", 55)
    dst.mergeFonts(srcfile)

    # The first merged glyph's data must survive the second merge (and the
    # freeing of the second source font).
    g = dst[ord('A')]
    if g.verticalVariants != "B.v C.v":
        raise ValueError("vertical variants were corrupted during merge")
    if g.verticalComponentItalicCorrection != 20:
        raise ValueError("vertical component italic correction was corrupted during merge")
    if g.verticalVariants == "E.v F.v":
        raise ValueError("merged glyph still shares MATH data with the source font")

    # A glyph merged in from the first font must be intact as well.
    g = dst[ord('E')]
    if g.verticalVariants != "X.v Y.v":
        raise ValueError("vertical variants were corrupted during second merge")
    if g.verticalComponentItalicCorrection != 99:
        raise ValueError("vertical component italic correction was corrupted during second merge")

    # Generating a TTF dumps the MATH table from the merged copies (the gv_len
    # path); this used to read the freed source-font data.
    out = os.path.join(tmpdirname, "mergefont_math_test.ttf")
    dst.generate(out)

    # Closing the merged font must free each glyph's own MATH data exactly once
    # (it used to free the source font's data a second time, a double free).
    dst.close()
