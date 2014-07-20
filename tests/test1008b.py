# test1008b.py - check "remove overlap" using font OverlapBugs.sfd

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
  'exclam': (  1, ''  ), #  33

  'at':     (  2, ''  ), #  64
  'A':      (  5, ''  ), #  65 
  'B':      (  5, ''  ), #  66 
  'C':      (  2, ''  ), #  67 
  'D':      (  2, ''  ), #  68 
  'E':      (  2, ''  ), #  69 
  'F':      (  3, ''  ), #  70 
  'G':      (  3, ''  ), #  71 
  'H':      (  3, ''  ), #  72 
  'I':      (  3, ''  ), #  73 
  'J':      (  1, ''  ), #  74 
  'K':      (  3, ''  ), #  75 
  'L':      (  2, ''  ), #  76 
  'M':      (  2, ''  ), #  77 
  'N':      (  2, ''  ), #  78 
  'O':      (  3, ''  ), #  79 
  'P':      (  3, ''  ), #  80 
  'Q':      (  1, ''  ), #  81 
  'R':      (  4, ''  ), #  82 
  'S':      (  2, ''  ), #  83 
  'T':      (  3, ''  ), #  84   why 3?
  'U':      (  1, ''  ), #  85 
  'V':      (  2, ''  ), #  86 
  'W':      (  3, ''  ), #  87 
  'X':      (  1, ''  ), #  88 
  'Y':      (  6, ''  ), #  89 
  'Z':      (  1, ''  ), #  90 

  'bracketleft':  (  2, ''  ), #  91 
  'backslash':    (  2, ''  ), #  92 
  'bracketright': (  2, ''  ), #  93 
  'asciicircum':  (  2, ''  ), #  94 
  'underscore':   ( 12, ''  ), #  95 
  'grave':        (  2, ''  ), #  96

  'a':      (  2, ''  ), #  97
  'b':      (  8, ''  ), #  98
  'c':      (  4, ''  ), #  99
  'd':      (  3, ''  ), # 100
  'e':      (  2, ''  ), # 101
  'f':      (  2, ''  ), # 102
  'g':      (  4, ''  ), # 103 doubled glyph overlapped
  'h':      (  3, ''  ), # 104
  'i':      (  6, ''  ), # 105
  'j':      (  5, ''  ), # 106 two of three splines duplicated
  'k':      (  2, ''  ), # 107
  'l':      (  2, ''  ), # 108
  'm':      (  2, ''  ), # 109
  'n':      (  3, ''  ), # 110
  'o':      (  2, ''  ), # 111
  'p':      (  2, ''  ), # 112
  'q':      (  2, ''  ), # 113
  'r':      (  4, ''  ), # 114
  's':      (  2, ''  ), # 115
  't':      (  2, ''  ), # 116
  'u':      (  3, ''  ), # 117
  'v':      (  4, ''  ), # 118
  'w':      (  5, ''  ), # 119
  'x':      (  5, ''  ), # 120
  'y':      (  3, ''  ), # 121
  'z':      (  3, ''  ), # 122

  'braceleft':    (  3, ''  ), # 123
  'bar':          (  3, ''  ), # 124
  'braceright':   (  2, ''  ), # 125
  'asciitilde':   (  3, ''  ), # 126
  'uni007F':      (  2, ''  ), # 127

  'Aring':                  (  3, ''  ), # 197

  'accordion.accOldEE':     ( 17, ''  ), # 256 
  'accordion.accStdbase':   (  4, ''  ), # 257 
  'accordion.accFreebase':  (  2, ''  ), # 258 
  'flags.d4':               (  3, ''  ), # 259 
  'flags.d5':               (  4, ''  ), # 260 
  'flags.d6':               (  5, ''  ), # 261 
  'flags.u5':               (  4, ''  ), # 262 
  'flags.u6':               (  5, ''  ), # 263 
  'scripts.lineprall':      (  6, ''  ), # 264 
  'scripts.varcoda':        (  6, ''  ), # 265 
  'scripts.coda':           (  3, ''  ), # 266 
}

expectedAfter = {
  'exclam': (  3, ''  ), #  33
  'at':     (  1, ''  ), #  64
  'A':      (  2, ''  ), #  65
  'B':      (  2, ''  ), #  66
  'C':      (  1, ''  ), #  67
  'D':      (  1, ''  ), #  68
  'E':      (  1, ''  ), #  69
  'F':      (  2, ''  ), #  70
  'G':      (  2, ''  ), #  71
  'H':      (  2, ''  ), #  72
  'I':      (  2, ''  ), #  73
  'J':      (  1, ''  ), #  74
  'K':      (  2, ''  ), #  75 # direction problem?
  'L':      (  1, ''  ), #  76 # direction problem?
  'M':      (  1, ''  ), #  77
  'N':      (  1, ''  ), #  78
  'O':      (  3, ''  ), #  79
  'P':      (  2, ''  ), #  80
  'Q':      (  2, ''  ), #  81
  'R':      (  2, ''  ), #  82
  'S':      (  1, ''  ), #  83
  'T':      (  1, ''  ), #  84
  'U':      (  1, ''  ), #  85
  'V':      (  1, ''  ), #  86
  'W':      (  3, ''  ), #  87
  'X':      (  1, ''  ), #  88
  'Y':      (  2, ''  ), #  89  # why not 1 or 3?
  'Z':      (  1, ''  ), #  90

  'bracketleft':  (  1, ''  ), #  91
  'backslash':    (  1, ''  ), #  92
  'bracketright': (  1, ''  ), #  93
  'asciicircum':  (  1, ''  ), #  94
  'underscore':   (  9, ''  ), #  95
  'grave':        (  2, ''  ), #  96  direction problems?

  'a':      (  2, ''  ), #  97  direction problems?
  'b':      (  4, ''  ), #  98
  'c':      (  3, ''  ), #  99  direction problems?
  'd':      (  1, ''  ), # 100
  'e':      (  1, ''  ), # 101
  'f':      (  1, ''  ), # 102
  'g':      (  2, ''  ), # 103
  'h':      (  5, ''  ), # 104
  'i':      (  8, ''  ), # 105
  'j':      (  3, ''  ), # 106 very bad result
  'k':      (  2, ''  ), # 107
  'l':      (  1, ''  ), # 108 direction problems?
  'm':      (  1, ''  ), # 109
  'n':      (  3, ''  ), # 110
  'o':      (  2, ''  ), # 111
  'p':      (  1, ''  ), # 112
  'q':      (  1, ''  ), # 113
  'r':      (  4, ''  ), # 114
  's':      (  2, ''  ), # 115
  't':      (  2, ''  ), # 116
  'u':      (  3, ''  ), # 117
  'v':      (  3, ''  ), # 118
  'w':      (  5, ''  ), # 119
  'x':      (  5, ''  ), # 120
  'y':      (  3, ''  ), # 121
  'z':      (  4, ''  ), # 122

  'braceleft':    (  3, ''  ), # 123
  'bar':          (  3, ''  ), # 124
  'braceright':   (  2, ''  ), # 125
  'asciitilde':   (  2, ''  ), # 126
  'uni007F':      (  1, ''  ), # 127

  'Aring':                  (  2, ''  ), # 197

  'accordion.accOldEE':     (  3, ''  ), # 256 
  'accordion.accStdbase':   (  5, ''  ), # 257 
  'accordion.accFreebase':  (  3, ''  ), # 258 
  'flags.d4':               (  2, ''  ), # 259 
  'flags.d5':               (  3, ''  ), # 260 
  'flags.d6':               (  4, ''  ), # 261 
  'flags.u5':               (  3, ''  ), # 262 
  'flags.u6':               (  4, ''  ), # 263 
  'scripts.lineprall':      (  1, ''  ), # 264 
  'scripts.varcoda':        (  5, ''  ), # 265 
  'scripts.coda':           (  5, ''  ), # 266 
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

print "  %d glyphs were seen in font" % (glyphsSeen)
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
