# test1008a.py - check "remove overlap" using font OverlapCounts1.sfd

# Check the results of "remove overlap" operations on individual glyphs
# of a font file.  The number of splines resulting from the operation
# is checked against the number expected.
#
# Other tests only probe to see whether "remove overlap" causes visible
# error messages or more dramatically program crashes.  Those don't 
# address the question "did we (seem to) get the correct result?"  
#
# While only eyeball inspection of the resulting glyph can determine 
# the actual correctness of "remove overlap", we can here at least
# check that the number of splines is as expected.
#
# The spline count can be checked both before and after the "remove
# overlap" operation.  Checking the count afterward is the important
# purpose of this test, but checking and having to determine the counts
# for the original glyphs can reveal important details that might be
# confusing the "remove overlap" operation.  For instance, some of GW's
# torture-test glyphs have duplicate outlines... :)


import sys, fontforge

showSummaryListing = True


# Capture summary of glyphs and their contours in a list

def makeNotesAboutGlyphs(iterGlyph, showList):

  notes = {}

  if showList:
    print "   GID Glyph Name   Contours   Contour closed flags"

  for glyphName in iterGlyph:
    glyph       = font1[glyphName]
    gid         = glyph.encoding
    glyphLayer  = glyph.layers[glyph.activeLayer]
    contourCount = len(glyphLayer)

    # create string describing 'closed' flags for glyph contours
    closedFlags = ""
    allClosed   = True
    for contour in glyphLayer:
      closedFlags += "T " if contour.closed else "F "
      allClosed = allClosed and contour.closed
    closedFlags = closedFlags[0:-1]

    notes[glyphName] = [gid, glyphName, contourCount, closedFlags, allClosed]

    if showList:
      glyphNameQ = "'" + glyphName + "'"
      print "  %4d  %-16s %3d    %s" % (gid, glyphNameQ, contourCount, closedFlags)

  if showList:
    print ""

  return notes



inputFilepath = sys.argv[1]
print "Opening input file '" + inputFilepath + "'"
font1 = fontforge.open(inputFilepath)

print "  Font '%s'  from  %s" % (font1.fontname, font1.path)
print ""


notesBefore = makeNotesAboutGlyphs(font1, showSummaryListing)


print "Applying \"remove overlap\" to all glyphs"

for glyphName in font1:
  glyph = font1[glyphName]
  glyph.removeOverlap()

print ""

# Gather the summary results of operation on all glyphs
notesAfter = makeNotesAboutGlyphs(font1, showSummaryListing)


# Each entry in the dictionary is made of:
#   glyph name        as key
#   expected count    of contours/splines composing glyph
#   closed flags      whether individual splines are marked 'closed';
#
# The closed flags string entry allows you to compare against abnormal
# splines.  All normal splines should be marked as closed.  And thus the
# string here would be "T T T T...".  But if an input or output glyph 
# is expected to have some _un_closed splines, you can specify that 
# like "T F T" for a 3 spline glyph with the second spline unclosed.
#
# Only the glyphs named in these two dictionaries will be checked
# If you don't want to check the original input glyphs, just leave the
# expectedBefore dictionary empty.  

expectedBefore = {
  '.notdef': (  2, ''  ),
  'C12341':  (  5, ''  ),
  'C12357':  (  6, ''  ),
  'C12360':  (  4, ''  ),
  'C12592':  (  5, ''  ),
  'C12860':  (  6, ''  ),
  'C12886':  (  6, ''  ),
  'C13128':  ( 10, ''  ),
  'C14659':  (  5, ''  ),
  'C15198':  (  3, ''  ),
  'C16452':  (  6, ''  ),
  'C17233':  (  4, ''  ),
  'C17504':  ( 21, ''  ),
  'C17765':  ( 13, ''  ),
  'C22067':  (  7, ''  ),
  'C90023':  ( 12, ''  ),
  'C90043':  (  6, ''  ),
  'C90084':  (  7, ''  ),
  'C90160':  (  6, ''  ),
  'C99999':  ( 10, ''  ),
}
expectedAfter = {
  '.notdef': (  2, ''  ),
  'C12341':  (  2, ''  ),
  'C12357':  (  5, ''  ),
  'C12360':  (  5, ''  ),
  'C12592':  (  4, ''  ),
  'C12860':  (  4, ''  ),
  'C12886':  (  4, ''  ),
  'C13128':  (  5, ''  ),
  'C14659':  (  7, ''  ),
  'C15198':  (  3, ''  ),
  'C16452':  (  5, ''  ),
  'C17233':  (  2, ''  ),
  'C17504':  ( 20, ''  ),
  'C17765':  ( 10, ''  ),
  'C22067':  (  3, ''  ),
  'C90023':  (  9, ''  ),
  'C90043':  (  2, ''  ),
  'C90084':  (  7, ''  ),
  'C90160':  (  9, ''  ),
  'C99999':  ( 10, ''  ),
}



print "Comparing before and after results..."

glyphsSeen    = 0
glyphsChecked = 0
problemsSeen  = 0

for glyphName in font1:
  glyph   = font1[glyphName]
  gid     = glyph.encoding
  glyphsSeen += 1
  glyphNameQ = "'" + glyphName + "':"

  (beforeGID, beforeName, beforeCount, beforeCloseds, beforeAllClosed) = notesBefore[glyphName]
  (afterGID,  afterName,  afterCount,  afterCloseds,  afterAllClosed)  = notesAfter[glyphName] 

  # Check glyphs before "remove overlap"
  # Guard against input of wrong/modified font file 
  if glyphName in expectedBefore:

    (beforeCountExpected, beforeClosedsExpected) = expectedBefore[glyphName]

    if beforeCountExpected != beforeCount:
      print "  %3d %-8s Expected 'before' count %d does not match actual count %d" % (gid, glyphNameQ, beforeCountExpected, beforeCount)
      problemsSeen += 1

    if beforeClosedsExpected != '':
      if beforeClosedsExpected != beforeCloseds:
        print "  %3d %-8s Expected 'before' closed flags does not match result: '%s' vs. '%s'" % (gid, glyphNameQ, beforeClosedsExpected, beforeCloseds)
        problemsSeen += 1

    else:
      if not beforeAllClosed:
        print "  %3d %-8s Expected all 'before' closed flags to be true, saw '%s'" % (gid, glyphNameQ, beforeCloseds)
        problemsSeen += 1


  # Check results after "remove overlap"
  if glyphName in expectedAfter:

    (afterCountExpected,  afterClosedsExpected)  = expectedAfter[glyphName] 

    if afterCountExpected != afterCount:
      print "  %3d %-8s Expected 'after' count %d does not match result %d" % (gid, glyphNameQ, afterCountExpected, afterCount)
      problemsSeen += 1

    if afterClosedsExpected != '':
      if afterClosedsExpected != afterCloseds:
        print "  %3d %-8s Expected 'after' closed flags does not match result: '%s' vs. '%s'" % (gid, glyphNameQ, afterClosedsExpected, afterCloseds)
        problemsSeen += 1

    else:
      if not afterAllClosed:
        print "  %3d %-8s Expected all 'after' closed flags to be true, saw '%s'" % (gid, glyphNameQ, afterCloseds)
        problemsSeen += 1
  
    glyphsChecked += 1

print "  %d result characters were checked" % (glyphsChecked)

if problemsSeen:
  print "  %d problems were detected - failing this test!" % (problemsSeen)
  #raise ValueError("  %d problems were detected - failing this test!" % (problemsSeen))
  sys.exit("  %d problems were detected - failing this test!" % (problemsSeen))

if glyphsChecked == 0:
  print "  %d glyphs were seen in font, but none were checked for result spline counts" % (glyphsSeen)

print "  No problems found - test successful"
sys.exit(0)

# vim:ft=python:ts=2:sw=2:et:is:hls:ss=10:tw=222:
