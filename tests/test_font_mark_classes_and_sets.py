"""Test getter and setter of font.markClasses and font.markSets"""
import fontforge


GLYPHS = ('A', 'B', 'C')
MARK_CLASSES = (('MarkClass-vowel', ('A',)),
                ('MarkClass-consonant', ('B', 'C')))
MARK_SETS = (('MarkSet-roman-numeral', ('C',)),
             ('MarkSet-letter', ('A', 'B', 'C')))

font = fontforge.font()
for name in GLYPHS:
    font.createMappedChar(name)

assert font.markClasses is None
assert font.markSets is None

font.markClasses = MARK_CLASSES
assert font.markClasses == MARK_CLASSES

del font.markClasses
assert font.markClasses is None

font.markClasses = ()
assert font.markClasses is None

font.markSets = MARK_SETS
assert font.markSets == MARK_SETS

del font.markSets
assert font.markSets is None

font.markSets = ()
assert font.markSets is None

font.close()
