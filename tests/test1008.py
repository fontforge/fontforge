# Test various Python APIs to examine all glyphs in a font
#
# Checking these API operations from python.c:
#
#   if integer in f:          - font has a glyph for this encoding value?
#   glyph = f[integer]        - get glyph object for encoding value index
#   for name in f:            - iterate through glyph names in font
#   glyph = f[name]           - get glyph object for glyph name key
#   for glyph in f.glyphs()   
#                             - iterate thru glyphs in GID order
#   for glyph in f.glyphs('encoding') 
#                             - iterate thru glyphs in encoding order
#   if name in f:             - font has a glyph for this glyph name?

import sys, fontforge

dontFailTest  = False

problemsSeen = 0

inputFilepath = sys.argv[1]
print("Opening input file '" + inputFilepath + "'")
font1 = fontforge.open(inputFilepath)

print("  Font '%s'  from  %s" % (font1.fontname, font1.path))
print("")

# We can get names in two different orderings, by encoding value order
# and by 'GID' order.  They can differ, as in OverlapBugs.sfd

glyphNamesInGIDOrder = []
glyphNamesInEncOrder = []


# This loops over a range of possible GID's, testing to each to see
#   if it is for a valid glyph.  Thus the tested operations are:
#         if enc in font1:          - 'contains' test
#           glyph = font1[enc]      - index by number

if True:
  print("Listing glyph names over range of encoding values:")

  # XXX use an API to get actual count?  then check that API? :)
  for enc in range(1024):
    if enc in font1:
      glyph = font1[enc]
      glyphName = glyph.glyphname 
      print("  {:3d} glyphName:   '{}'".format(enc, glyphName))
      glyphNamesInEncOrder.append(glyphName)
      # XXX check that 'enc' matches glyph.encoding ?

  print("")

# This loops using an iterator running over all glyphs in a font, 
#   in GID order, and returning the glyph name.
#   Here the tested operations are:
#         for glyphName in font1:     - font iterator for names
#           glyph = font1[glyphName]  - index by string key

# Allow testing against either a known list of glyph names, or 
#   using the iterator from the font object.

iterGlyph = font1
#iterGlyph = ( '.notdef', 'C12341', 'C12357', 'C12360', 'C12592', 'C12860' )


if True:
  print("Listing glyph names using \"for glyphName in font:\" iterator:")

  for glyphName in iterGlyph:
    glyph = font1[glyphName]
    enc   = glyph.encoding
    print("  {:3d} glyphName:   '{}'".format(enc, glyphName))
    glyphNamesInGIDOrder.append(glyphName)

  # While the order of names may differ between encoding and GID ordering,
  # these two lists should at least have the same count of names
  if len(glyphNamesInEncOrder) != len(glyphNamesInGIDOrder):
    print("*Error: the list of glyph names saved from using font iterator does not match the lists from other API usage")
    print("  Previous list count: {:d}".format(len(glyphNamesInEncOrder)))
    print("     this list count:  {:d}".format(len(glyphNamesInGIDOrder)))
    print("  Previous list:       {}".format(glyphNamesInEncOrder))
    print("     this list:        {}".format(glyphNamesInGIDOrder))
    problemsSeen += 1

  print("")



if True:
  print("Listing glyph names using font.glyphs() iterator:")

  glyphNames = []

  for glyph in font1.glyphs():
    glyphName = glyph.glyphname 
    enc   = glyph.encoding
    print("  {:3d} glyphName:   '{}'".format(enc, glyphName))
    glyphNames.append(glyphName)


  if glyphNamesInGIDOrder != glyphNames:
    print("*Error: the list of glyph names saved from using iterator font.glyphs() does not match the lists from other API usage")
    print("  Previous list count: {:d}".format(len(glyphNamesInGIDOrder)))
    print("     this list count:  {:d}".format(len(glyphNames)))
    print("  Previous list:       {}".format(glyphNamesInGIDOrder))
    print("     this list:        {}".format(glyphNames))
    problemsSeen += 1

  print("")

  print("Listing glyph names using font.glyphs('encoding') iterator:")

  glyphNames = []

  for glyph in font1.glyphs('encoding'):
    glyphName = glyph.glyphname 
    enc = glyph.encoding
    print("  {:3d} glyphName:   '{}'".format(enc, glyphName))
    glyphNames.append(glyphName)


  if glyphNamesInEncOrder != glyphNames:
    print("*Error: the list of glyph names saved from using iterator font.glyphs('encoding') does not match the lists from other API usage")
    print("  Previous list count: {:d}".format(len(glyphNamesInEncOrder)))
    print("     this list count:  {:d}".format(len(glyphNames)))
    print("  Previous list:       {}".format(glyphNamesInEncOrder))
    print("     this list:        {}".format(glyphNames))
    problemsSeen += 1

  print("")


if True:
  print("Listing glyph names in list and checking with \"glyphName in font\":")

  glyphNames = []

  for glyphName in glyphNamesInEncOrder:
    if glyphName in font1:
      glyph = font1[glyphName]
      enc   = glyph.encoding
      print("  {:3d} glyphName:   '{}'".format(enc, glyphName))
      glyphNames.append(glyphName)
    else:
      print("*Warning: known glyph name '{:s}' not found using test \"'{:s}' in font\"".format(glyphName, glyphName))

  if glyphNamesInEncOrder != glyphNames:
    print("*Error: the list of glyph names found using \"glyphName in font\" does not match the lists from other API usage")
    print("  Previous list count: {:d}".format(len(glyphNamesInEncOrder)))
    print("     this list count:  {:d}".format(len(glyphNames)))
    print("  Previous list:       {}".format(glyphNamesInEncOrder))
    print("     this list:        {}".format(glyphNames))
    problemsSeen += 1
      
  print("")


if problemsSeen:
  if dontFailTest:
    print("  %d problem results were detected - would fail this test..." % (problemsSeen))
    print("      but 'no fail' flag set.")
    sys.exit(0)
  else:
    print("  %d problem results were detected - failing this test!" % (problemsSeen))
    sys.exit("  %d failing glyphs were detected - failing this test!" % (problemsSeen))
else:
  print("  No problems found - test successful")
  sys.exit(0)

# vim:ft=python:ts=2:sw=2:et:is:hls:ss=10:tw=222:
