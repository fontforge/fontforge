import sys
import fontforge

f1 = fontforge.open(sys.argv[1])
f1.generate('CMAPEncTest.ttf')
f2 = fontforge.open('CMAPEncTest.ttf')

f1.encoding = 'UnicodeBmp'
# f2[172].unicode = 300 # Uncomment to verify test logic
f2.encoding = 'UnicodeBmp'

enc1 = { g.encoding: g for g in f1.glyphs('encoding') }
enc2 = { g.encoding: g for g in f2.glyphs('encoding') }

for e, g1 in enc1.items():
    g2 = enc2.get(e)
    assert( g2 and g1.glyphname == g2.glyphname )
