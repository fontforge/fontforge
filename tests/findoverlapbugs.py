import sys
import fontforge

# This script looks for bugs in remove overlap. The only way I know to do that
# is to pile shapes on top of one another and see if remove overlap has problems
# with the combination. To this end we take an already existing font (as a
# command line argument) and test all combinations of two glyphs. Failure
# notices should appear on stderr with the two glyphs that cause the problem
# indicated by "<glyphname1>__<glyphname2>".
#  Even nastier is to reverse the direction of one of the glyphs. Currently
# that is commented out.

font = fontforge.open(sys.argv[1])
font.encoding = "Original"
font.unlinkReferences()
testf = fontforge.font()
testf.em = font.em
testf.layers["Fore"].is_quadratic = font.layers["Fore"].is_quadratic
testg = testf.createChar(0x65)

i=0
cnt = len(font)
while ( i<cnt ) :
    g1 = font[i]
    fore1 = g1.foreground
    j=i
    while ( j<cnt ) :
	g2 = font[j]
	fore2 = g2.foreground
	testg.foreground = fore1 + fore2
	testg.glyphname = g1.glyphname + "__" + g2.glyphname
	testg.removeOverlap()
#Contour order seems to matter... (It shouldn't, but it can)
	testg.foreground = fore2 + fore1
	testg.glyphname = g2.glyphname + "__" + g1.glyphname
	testg.removeOverlap()
#And then reversing the direction
#	testg.foreground = fore1 + fore2.reverseDirection()
#	testg.glyphname = g1.glyphname + "__" + g2.glyphname + "_R"
#	testg.removeOverlap()
	j += 1
    i+=1
