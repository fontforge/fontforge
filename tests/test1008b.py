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
showContoursOnFail = False
dontFailTest  = True


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
  #glyph.simplify()

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

  'braceleft':      (  3, ''  ), # 123
  'bar':            (  3, ''  ), # 124
  'braceright':     (  2, ''  ), # 125
  'asciitilde':     (  3, ''  ), # 126
  'uni007F':        (  2, ''  ), # 127

  'uni00A0':        (  2, ''  ), # 160
  'exclamdown':     (  4, ''  ), # 161
  'cent':           (  4, ''  ), # 162
  'sterling':       (  4, ''  ), # 163
  'currency':       (  2, ''  ), # 164
  'yen':            (  4, ''  ), # 165
  'brokenbar':      (  5, ''  ), # 166
  'section':        (  6, ''  ), # 167
  'dieresis':       (  5, ''  ), # 168
  'copyright':      (  4, ''  ), # 169
  'ordfeminine':    (  2, ''  ), # 170
  'guillemotleft':  (  2, ''  ), # 171
  'logicalnot':     (  2, ''  ), # 172
  'uni00AD':        (  7, ''  ), # 173
  'registered':     (  3, ''  ), # 174
  'macron':         (  4, ''  ), # 175
  'degree':         (  2, ''  ), # 176
  'plusminus':      (  2, ''  ), # 177
  'uni00B2':        (  2, ''  ), # 178
  'uni00B3':        (  3, ''  ), # 179
  'acute':          (  4, ''  ), # 180
  'uni00B5':        (  5, ''  ), # 181
  'paragraph':      (  3, ''  ), # 182
  'periodcentered': (  4, ''  ), # 183
  'cedilla':        (  2, ''  ), # 184
  'uni00B9':        (  2, ''  ), # 185
  'ordmasculine':   (  2, ''  ), # 186
  'guillemotright': (  2, ''  ), # 187
  'onequarter':     (  2, ''  ), # 188
  'onehalf':        (  3, ''  ), # 189
  'threequarters':  (  2, ''  ), # 190
  'questiondown':   (  2, ''  ), # 191
  'Agrave':         (  2, ''  ), # 192
  'Aacute':         (  2, ''  ), # 193
  'Acircumflex':    (  2, ''  ), # 194
  'Atilde':         (  4, ''  ), # 195
  'Adieresis':      (  2, ''  ), # 196
  'Aring':          (  3, ''  ), # 197

  'AE':             (  4, ''  ), # 198
  'Ccedilla':       (  2, ''  ), # 199
  'Egrave':         (  2, ''  ), # 200
  'Eacute':         (  3, ''  ), # 201
  'Ecircumflex':    (  4, ''  ), # 202
  'Edieresis':      (  3, ''  ), # 203
  'Igrave':         (  4, ''  ), # 204
  'Iacute':         (  3, ''  ), # 205
  'Icircumflex':    (  2, ''  ), # 206
  'Idieresis':      (  6, ''  ), # 207
  'Eth':            (  4, ''  ), # 208
  'Ntilde':         (  2, ''  ), # 209
  'Ograve':         (  2, ''  ), # 210
  'Oacute':         (  2, ''  ), # 211


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

  'uni00A0':        (  1, ''  ), # 160
  'exclamdown':     (  3, ''  ), # 161
  'cent':           (  3, ''  ), # 162
  'sterling':       (  3, ''  ), # 163
  'currency':       (  2, ''  ), # 164
  'yen':            (  3, ''  ), # 165
  'brokenbar':      (  4, ''  ), # 166
  'section':        (  6, ''  ), # 167
  'dieresis':       (  9, ''  ), # 168
  'copyright':      (  4, ''  ), # 169
  'ordfeminine':    (  2, ''  ), # 170
  'guillemotleft':  (  1, ''  ), # 171
  'logicalnot':     (  1, ''  ), # 172
  'uni00AD':        (  7, ''  ), # 173
  'registered':     (  4, ''  ), # 174
  'macron':         (  2, ''  ), # 175
  'degree':         (  1, ''  ), # 176
  'plusminus':      (  1, ''  ), # 177
  'uni00B2':        (  3, ''  ), # 178
  'uni00B3':        (  2, ''  ), # 179
  'acute':          (  2, ''  ), # 180
  'uni00B5':        (  3, ''  ), # 181
  'paragraph':      (  3, ''  ), # 182
  'periodcentered': (  4, ''  ), # 183
  'cedilla':        (  1, ''  ), # 184
  'uni00B9':        (  1, ''  ), # 185
  'ordmasculine':   (  1, ''  ), # 186
  'guillemotright': (  1, ''  ), # 187
  'onequarter':     (  1, ''  ), # 188
  'onehalf':        (  2, ''  ), # 189
  'threequarters':  (  1, ''  ), # 190
  'questiondown':   (  1, ''  ), # 191
  'Agrave':         (  3, ''  ), # 192
  'Aacute':         (  1, ''  ), # 193
  'Acircumflex':    (  2, ''  ), # 194
  'Atilde':         (  5, ''  ), # 195
  'Adieresis':      (  2, ''  ), # 196
  'Aring':          (  2, ''  ), # 197

  'AE':             (  5, ''  ), # 198
  'Ccedilla':       (  4, ''  ), # 199
  'Egrave':         (  2, ''  ), # 200
  'Eacute':         (  2, ''  ), # 201
  'Ecircumflex':    (  6, ''  ), # 202
  'Edieresis':      (  5, ''  ), # 203
  'Igrave':         (  6, ''  ), # 204
  'Iacute':         (  5, ''  ), # 205
  'Icircumflex':    (  1, ''  ), # 206
  'Idieresis':      (  5, ''  ), # 207
  'Eth':            (  3, ''  ), # 208
  'Ntilde':         (  1, ''  ), # 209
  'Ograve':         (  1, ''  ), # 210
  'Oacute':         (  1, ''  ), # 211

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
  beforeProblemSeen = afterProblemSeen = False
  glyphNameQ = "'" + glyphName + "':"

  (beforeGID, beforeName, beforeCount, beforeCloseds, beforeAllClosed) = notesBefore[glyphName]
  (afterGID,  afterName,  afterCount,  afterCloseds,  afterAllClosed)  = notesAfter[glyphName] 

  # Check glyphs before "remove overlap"
  # Guard against input of wrong/modified font file 
  if glyphName in expectedBefore:

    (beforeCountExpected, beforeClosedsExpected) = expectedBefore[glyphName]

    if beforeCountExpected != beforeCount:
      print " %3d %-8s Expected 'before' count %d does not match result %d" % (gid, glyphNameQ, beforeCountExpected, beforeCount)
      beforeProblemSeen = True

    if beforeClosedsExpected != '':
      if beforeClosedsExpected != beforeCloseds:
        print " %3d %-8s Expected 'before' closed flags does not match result: '%s' vs. '%s'" % (gid, glyphNameQ, beforeClosedsExpected, beforeCloseds)
        beforeProblemSeen = True

    else:
      if not beforeAllClosed:
        print " %3d %-8s Expected all 'before' closed flags to be true, saw '%s'" % (gid, glyphNameQ, beforeCloseds)
        beforeProblemSeen = True

    if beforeProblemSeen:
      problemsSeen += 1
      glyphNameQ = "'" + beforeName + "'"
      print "  %4d  %-16s %3d    %s" % (beforeGID, glyphNameQ, beforeCount, beforeCloseds)


  # Check results after "remove overlap"
  if glyphName in expectedAfter:

    (afterCountExpected,  afterClosedsExpected)  = expectedAfter[glyphName] 

    if afterCountExpected != afterCount:
      print " %3d %-8s Expected 'after' count %d does not match result %d" % (gid, glyphNameQ, afterCountExpected, afterCount)
      afterProblemSeen = True

    if afterClosedsExpected != '':
      if afterClosedsExpected != afterCloseds:
        print " %3d %-8s Expected 'after' closed flags does not match result: '%s' vs. '%s'" % (gid, glyphNameQ, afterClosedsExpected, afterCloseds)
        afterProblemSeen = True

    else:
      if not afterAllClosed:
        print " %3d %-8s Expected all 'after' closed flags to be true, saw '%s'" % (gid, glyphNameQ, afterCloseds)
        afterProblemSeen = True
  
    glyphsChecked += 1

    if afterProblemSeen:
      problemsSeen += 1
      glyphNameQ = "'" + afterName + "'"
      print "  %4d  %-16s %3d    %s" % (afterGID, glyphNameQ, afterCount, afterCloseds)
      if showContoursOnFail:
        glyphLayer  = glyph.layers[glyph.activeLayer]
        print glyphLayer

print ""
print "  %d glyphs were seen in font" % (glyphsSeen)
print "  %d result glyphs were checked" % (glyphsChecked)

if glyphsChecked == 0:
  print "  %d glyphs were seen in font, but none were checked for result spline counts" % (glyphsSeen)

if problemsSeen:
  if dontFailTest:
    print "  %d failing glyphs were detected - would fail this test..." % (problemsSeen)
    print "      but 'do not fail' flag set."
    sys.exit(0)
  else:
    print "  %d failing glyphs were detected - failing this test!" % (problemsSeen)
    sys.exit("  %d failing glyphs were detected - failing this test!" % (problemsSeen))
else:
  print "  No problems found - test successful"
  sys.exit(0)

# vim:ft=python:ts=2:sw=2:et:is:hls:ss=10:tw=222:
