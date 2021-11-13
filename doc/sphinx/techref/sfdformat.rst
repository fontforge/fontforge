Spline Font Database
====================

.. warning::

   This page is often out of date. I thought it was a correct on 16 Feb 2011. A
   :doc:`changelog <sfdchangelog>` file is available. Even if out of date it
   should be helpful, but if you really need to know the current format look at
   `fontforge/sfd.c <https://github.com/fontforge/fontforge/blob/master/fontforge/sfd.c>`_
   and see what it parses. Some descriptions of esoteric features are not
   complete, but I hope they give you enough hints that you can figure the
   format out (especially after looking it up in sfd.c)

.. epigraph::

   | **TYSON:**
   | What is the meaning of this?
   |
   | **THOMAS**
   | That's the most relevant
   | Question in the world.

   -- *The Lady's not for Burning*
   -- Christopher Fry

The sfd [#sfdext]_ format evolves over time. I hope the current parser can handle old sfd
formats. Most changes have been small, but in Feb 2008 I allowed multiple layers
in fonts and this turned out to be an incompatible change and I raised the sfd
version number to 3, and in March of 2007 the way fontforge handled lookups and
features changed radically (and I bumped the SFD file version number for the
first time) and sfd files created before then are better understood if you look
at an
`older version of this document <https://github.com/fontforge/fontforge/commits/master/htdocs/sfdformat.html>`_.

FontForge's sfd files are ASCII files (so they can be copied easily
across the internet and so that diffs are somewhat meaningful). They contain a
full description of your font. As of 14 May 2008 there is a registered (with
IANA) MIME type for them
`application/vnd.font-fontforge-sfd <http://www.iana.org/assignments/media-types/application/>`_.

They are vaguely modeled on bdf files. The first few lines contain general font
properties, then there's a section for each character, then a section for each
bitmap font.

.. contents::
   :depth: 2
   :backlinks: none

.. warning::

   It is tempting to cut and paste information from one sfd file to another.
   This is usually ok, but there are a couple of cases than need to be avoided:

* If you copy a glyph with truetype instructions, those instructions may call
  subroutines defined in the old font but not the new. Unexpected things may
  happen when the glyph is grid fit (including system crashes)
* If you copy a substitution (ligature, kern, contextual chaining, ...) item from
  one font to another, the substitution contains the name of a lookup-subtable.
  Bad things will probably happen if the lookup subtable isn't in the font.
* If you copy a glyph containing references -- these are done by glyph index
  (which is probably different from font to font). You may end up referring to the
  wrong glyph.


.. _sfdformat.Font-Header:

Font Header
-----------

Here is an example of what the first few lines look like:

::

   SplineFontDB: 3.0
   FontName: Ambrosia
   FullName: Ambrosia
   FamilyName: Ambrosia
   DefaultBaseFilename: Ambrosia-1.0
   Weight: Medium
   Copyright: Copyright (C) 1995-2000 by George Williams
   Comments: This is a funny font.
   UComments: "This is a funny font."
   FontLog: "Create Jan 2008"
   Version: 001.000
   ItalicAngle: 0
   UnderlinePosition: -133
   UnderlineWidth: 20
   Ascent: 800
   Descent: 200
   sfntRevision: 0x00078106
   WidthSeparation: 140
   LayerCount: 4
   Layer: 0 0 "Back" 1
   Layer: 1 1 "Fore" 0
   Layer: 2 0 "Cubic_Fore" 0
   Layer: 3 0 "Test" 1
   DisplaySize: -24
   DisplayLayer: 1
   AntiAlias: 1
   WinInfo: 64 16 4
   FitToEm: 1
   UseUniqueID: 0
   UseXUID: 1
   XUID: 3 18 21
   Encoding: unicode
   Order2: 1
   OnlyBitmaps: 0
   MacStyle: 0
   TeXData: 1 10485760 0 269484 134742 89828 526385 1048576 89828
   CreationTime: 1151539072
   ModificationTime: 11516487392
   GaspTable 3 8 2 16 1 65535 3 0
   DEI: 91125
   ExtremaBound: 30

The first line just identifies the file as an sfd file and provides a version
number. IT MUST BE FIRST in the file. The rest of the file is basically a set of
keyword value pairs. Within a given section, order is largely irrelevant. The
next few lines give the various different names that postscript allows fonts to
have. Then some fairly self-explanatory items (if they don't make sense, look
them up in the :doc:`font info </ui/dialogs/fontinfo>` dlg). A few things need some
explanation:

.. object:: Comments

   This is deprecated. A string of ASCII characters

.. object:: UComments

   New format for font comments. A string of utf7 characters.

.. object:: FontLog

   A string of utf7 characters.

.. object:: TeXData

   These are the TeX font parameters (and some similar info). The first number
   is 1,2 or 3 and indicates that the font is a text, math or math ext font. The
   next number is the design pointsize (times (1<<20)). Then follow the font
   parameters. These values are usually in TeX fix_word format where there is a
   binary point after the first 20 binary digits (so to get the number divide by
   (1<<20)).

.. object:: sfntRevision

   This is the revision number field of the 'head' table of an sfnt. It is
   stored in hex in a 16.16 fixed number (that is, a 32 bit number where the
   binary point is after the 16th binary bit).

.. object:: WidthSeparation

   This is internal information that the user never sees directly. It indicates
   the most recent value for the desired separation between glyphs that was used
   in the AutoWidth command. It is also used as a default for the separation in
   AutoKern.

.. object:: LayerCount

   The number of layers in a font, must be at least 2.

.. object:: Layer

   One entry for each layer to name it and describe its splines «Layer: 1 1
   "Fore" 0» means this is layer 1, it has quadratic splines, is named "Fore"
   and is not a background layer, while «Layer: 2 0 "Cubic_Fore" 0» means this
   is layer 2, it does not have quadratic splines (so it has cubic), is named
   "Cubic_Fore" and is also not a background layer.

   Layer <layer-number> <quadratic-flag> <name> [<background-flag>]

.. object:: DisplaySize

   This is the number of pixels per em that will be used by default to display
   the font in fontviews (it may be changed of course). Negative numbers mean to
   rasterize the display from the outlines, positive numbers mean to use a
   prebuilt bitmap font of that size.

.. object:: DisplayLayer

   The layer that should be displayed by default on opening the font.

.. object:: AntiAlias

   Whether the fontview should display the font as antialiased or black and
   white. (AntiAliased looks better, but will be slower)

.. object:: FitToEm

   Controls whether Fit to Em is checked by default in a fontview that displays
   this font.

.. object:: WinInfo

   Has three pieces of data on the default display of windows containing this
   font. The first datum says that the window should be scrolled so that glyph
   at encoding 64 should be visible, the second that the window should have 16
   character columns horizontally, and the last that there should be 4 character
   rows vertically.

.. object:: Encoding

   For normal fonts this will be one of the names (or a close approximation
   thereto) that appears in the Encoding pulldown list. CID keyed fonts will not
   have encodings. Instead they'll have something like:

   ::

      Registry: Adobe
      Ordering: japan1
      Supplement: 4
      CIDVersion: 1.2

.. object:: CreationTime

.. object:: ModificationTime

   These two dates are expressed as seconds since 00:00:00, 1 January, 1970 --
   standard unix dates.

.. object:: GaspTable

   The first number following the keyword gives the number of ppem/flag pairs on
   the line. The next two numbers are the first ppem and first flag. The last
   number is gasp table version.

.. object:: UseXUID

   Nowadays Adobe says XUID is deprecated. If this flag is set then fontforge
   will still generate an XUID entry for a postscript font.

.. object:: DEI

   It's too hard to explain, see the minutes of the CalTech OddHack committee
   from 15 Jan 1980. You can safely ignore it.

.. object:: ExtremaBound

   Adobe says that short splines are allowed to have internal extrema, but that
   big splines are not. But they don't define "big". This allows the user to
   specify that number. Splines where the distance between end-points is longer
   than this number will be checked for extrema.

For WOFF files

::

   woffMajor: 7
   woffMinor: 504
   woffMetadata: "<?xml version+AD0AIgAA-1.0+ACIA encoding ..."

.. object:: woffMajor

   The major version number to be stored in a woff file.

.. object:: woffMinor

   The minor version number of the woff file.

.. object:: woffMetadata

   Metadata for the woff file, stored in UTF7.

For UFO files

::

   UFOAscent: 697
   UFODescent: -154

.. object:: UFOAscent

   The value of the "ascender" field in the fontinfo.plist file of a UFO font.

.. object:: UFODescent

   The value of the "descender" field in the fontinfo.plist file of a UFO font.

Some fonts will have some TrueType information in them too (look at the
`truetype spec <http://www.microsoft.com/typography/tt/tt.htm>`_ for the
meanings of these, they usually live in the OS/2, hhea, or vhea tables).

::

   FSType: 4
   PFMFamily: 17
   TTFWeight: 400
   TTFWidth: 5
   Panose: 2 0 5 3 0 0 0 0 0 0
   LineGap: 252
   OS2LineGap: 252
   VLineGap: 0
   OS2Vendor: 'PfEd'
   OS2FamilyClass: 2050
   OS2Version: 4
   OS2_WeightWidthSlopeOnly: 1
   OS2_UseTypoMetrics: 1
   OS2CodePages: 6000009f.9fd70000
   OS2UnicodeRanges: 800002ef.50002049.00000000.00000000

The following items also come from the OS/2 and hhea tables but are slightly
more complex. The keywords are paired, so ``HheadAscent`` and ``HheadAOffset``
work together. If the offset keyword is 1 (true) then the other keyword is
treated as a value relative to what FontForge thinks should be the correct
value, FontForge will calculate what it thinks the value should be and then will
add the value specified in the keyword. So in the example below, FF will figure
out what it thinks ``HheadAscent`` should be and then add "0" to it to get the
value stored in a truetype font's OS/2 table. However if the Offset flag is set
to 0 (false) then the Ascent would be used exactly as specified.

::

   HheadAscent: 0
   HheadAOffset: 1
   HheadDescent: 0
   HheadDOffset: 1
   OS2TypoAscent: 0
   OS2TypoAOffset: 1
   OS2TypoDescent: 0
   OS2TypoDOffset: 1
   OS2WinAscent: 0
   OS2WinAOffset: 1
   OS2WinDescent: 0
   OS2WinDOffset: 1

These represent different definitions of ascent and descent that are stored in
various places in the truetype file (Horizontal header and OS/2 tables).

The OS/2 table contains information on the position of subscripts, superscripts
and strike throughs:

::

   OS2SubXSize: 1351
   OS2SubYSize: 1228
   OS2SubXOff: 0
   OS2SubYOff: -446
   OS2SupXSize: 1351
   OS2SupYSize: 1228
   OS2SupXOff: 0
   OS2SupYOff: 595
   OS2StrikeYSize: 143
   OS2StrikeYPos: 614

The MacStyle field (if present) indicates whether the font is bold, italic,
condensed, extended, etc.

Some fonts will have PostScript specific information contained in the Private
dictionary (the value is preceded by an integer holding the number of
characters needed for the string representation. It makes reading the file
slightly faster, but is ugly. I should not have done that, but too late now).

::

   BeginPrivate: 1
   BlueValues 23 [-19 0 502 517 750 768]
   EndPrivate

Some fonts may have python data:

::

   PickledData: "I3
   ."

This is arbetrary python pickled data (protocol=0) which got set by a python
script. FontForge stores it as a string. If there are either double quotes or
backslashes inside the string they will be preceded by a backslash.

If your font has any lookups

::

   Lookup: 6 0 0 "calt"  {"calt-1"  } ['calt' ('DFLT' <'dflt' > 'latn' <'dflt' > ) ]
   Lookup: 1 0 0 "'smcp' Lowercase to Small Capitals in Latin lookup 0"  {"'smcp' Lowercase to Small Capitals in Latin lookup 0"  } ['smcp' ('latn' <'dflt' > ) ]
   Lookup: 4 0 1 "'liga' Standard Ligatures in Latin lookup 1"  {"'liga' Standard Ligatures in Latin lookup 1"  } ['liga' ('latn' <'dflt' > ) ]
   Lookup: 258 0 0 "'kern' Horizontal Kerning in Latin lookup 0"  {"'kern' Horizontal Kerning in Latin lookup 0" [150,0,0]  } ['kern' ('latn' <'dflt' > ) ]

All entries in the lookup list start with the "Lookup:" keyword. They are
followed by a lookup type, and flags, and the save-in-afm flag. Then within
curly braces is a list of all subtable names in this lookup. Then within backets
a list of all features each followed (within parens) by a list of all scripts
each followed (within brockets) by a list of all languages. (the lookup flags
field is now a 32 bit number, the low order 16 bits being the traditional flags,
and the high order being the mark attachment set index, if any).

GSUB single substitution subtable names may be followed by a pair parentheses
containing a utf7 string with the default suffix used for this subtable.

Kerning subtable names may be followed by a "(1)" to indicated they are vertical
kerning, or by a pair of brackets containing three numbers. These numbers
represent default values for autokerning in this subtable, the first is the
desired separation between glyphs, the next is the minimum (absolute) value that
will generate a kerning pair (kerning by 1 em unit isn't interesting and if
that's what autokern comes up with, there is really no point to it and it wastes
time), and the last is a set of bit flags: if the number is odd then it means
separation is based on closest approach (touching), if the number has bit 2 set,
then only negative (closer) kerning values will be generated by autokerning and
if the number has bit 4 set then no auto- kerning will happen at all.

The order in which lookups are applied is the order listed here. The order in
which subtables are applied is the order listed here.

If your font has any kerning classes

::

   KernClass2: 31 64 2 "'kern' Horizontal Kerning in Latin lookup 0"
    1 F
    41 L Lacute glyph78 Lcommaaccent Ldot Lslash
    1 P
   ...
    6 hyphen
    5 space
   ...
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -152 -195 -152 -225 0 0 0 0 0 0 0 0 0 0 0 0 0 -145 -145 -130
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 -130 0 0 0 0 0 0 0 0 0 0 -145 0 -115 0 0 0 0 -65 0 -140 -120 -120
   ...

The first line says that this Kerning Class has 31 different classes for the
first character, and 64 for the second. It lives in the lookup subtable named
"'kern' Horizontal Kerning in Latin lookup 0".. The next line says that the
first character class of the first character (numbered 1, class 0 is reserved
and usually is not defined) consists of only one character "F" (the number in
front is the string length of the line. It speeds up processing the sfd file but
has no semantic content). The next line is for class 2 of the first character,
it has more characters in it and a longer string length. After 30 entries we
start on the classes for the second character. They look exactly like classes
for the first character. After all the second character classes have been
defined we have an array of numbers, <char1 class cnt>*<char2 class cnt> of them
in fact. This specifies the amount of kerning that should be placed between a
characters of the given classes of left and right characters (ie. if char1 was
in left class 2 and char2 was in right class 4 then we would index this array
with 3*<char2 class cnt> + 4).

In some cases it is possible to specify class 0 of the first glyph in a kerning
by classes entry (but not class 0 of the second glyph). In this case there will
be a plus sign after the count of classes for the first glyph. Then the first
list of names will be class 0.

You may find :ref:`device <sfdformat.device-table>` tables interspersed among
the kerning offsets array:

::

   ...
   0 {} 0 {} 0 {} ...
   -145 {12-13 -1,1} -145 {} -130 {8-9 -1,-1} ...

If your font has GDEF Mark attachment classes or sets these look like

::

   MarkAttachClasses: 2
   "ABClass" 3 A B
   MarkAttachSets: 2
   "ABSet" 3 A B
   "ASet" 1 A

In the example above there are (sort of) 2 mark attachment classes, but class 0
is always empty and isn't listed. So there's really one class. It is named (the
name is a FontForge thing, not exported to opentype) "ABClass", is 3 characters
long and is "A B".

Similarly, there are 2 mark attachment sets. Here set 0 is used, and must be
specified. Set 0 is called "ABSet" is 3 characters long and is "A B", while set
1 is called "ASet", is 1 character long and is "A".

If your font has any baseline data

::

   BaseHoriz: 3 'hang' 'ideo' 'romn'
   BaseScript: 'cyrl' 2  1405 -288 0
   BaseScript: 'grek' 2  1405 -288 0
   BaseScript: 'latn' 2  1405 -288 0 { 'dflt' -576 1913} { 'ENG ' -576 1482} { 'VIT ' -578 2150}

The BaseHoriz (or BaseVert) line indicates how many and which baselines are
active for this axis (Horizontal or Vertical). There is one BaseScript line for
each script. The first number after it indicates which baseline is the default
baseline for this script, subsequent numbers indicate how other baselines are
configured with respect to the default one. Language specific information
appears inside {} pairs. (feature specific information would be in {} pairs
inside the language specific curly braces).

If your font has any JSTF (justification) data

::

   Justify: 'arab'
   JstfExtender: afii57440 afii5739
   Justify: 'latn'
   JstfLang: 'dflt' 5
   JstfPrio:
   JstfMaxShrink: "JSTF shrinkage max at priority 0 #0 for dflt in latn"
   JstfMaxExtend: "JSTF extension max at priority 0 #1 for dflt in latn"
   JstfPrio:
   JstfEnableShrink: "'mark' Mark Positioning in Latin lookup 5"  "'kern' Horizontal Kerning in Latin lookup 6"  "'kern' Horizontal Kerning in Cyrillic lookup 7"
   JstfPrio:
   JstfEnableShrink: "'liga' Standard Ligatures in Latin lookup 10"  "'alig' Ancient Ligatures in Latin lookup 11"  "'liga' Standard Ligatures in Latin lookup 12"
   JstfDisableExtend: "'liga' Standard Ligatures in Latin lookup 10"  "'alig' Ancient Ligatures in Latin lookup 11"  "'liga' Standard Ligatures in Latin lookup 12"
   JstfPrio:
   JstfMaxShrink: "JSTF shrinkage max at priority 3 #2 for dflt in latn"
   JstfMaxExtend: "JSTF extension max at priority 3 #3 for dflt in latn"
   JstfPrio:
   JstfMaxShrink: "JSTF shrinkage max at priority 4 #4 for dflt in latn"
   JstfMaxExtend: "JSTF extension max at priority 4 #5 for dflt in latn"
   Justify: 'cyrl'
   JstfLang: 'dflt' 1
   JstfPrio:
   JstfMaxShrink: "JSTF shrinkage max at priority 0 #6 for dflt in cyrl"
   JstfMaxExtend: "JSTF extension max at priority 0 #7 for dflt in cyrl"
   EndJustify

A block of justification information begins with a ``Justify:`` keyword and is
followed by a script tag. There may be several ``Justify:`` instances if information is
provided for several scripts, the final block must be terminated with a
``EndJustify`` keyword.

Within a block extender glyphs (kashidas) may be specified with a
``JstfExtender:`` keyword followed by a list of glyph names.

Each language within the script is started by a ``JstfLang:`` keyword and is
followed by a language tag and a count of the number of priority levels.

Each priority level is started with a ``JstfPrio:`` keyword, after which you may
find any of the keywords
``JstfEnableShrink, JstfDisableShrink, JstfMaxShrink, JstfEnableExtend, JstfDisableExtend, JstfMaxExtend``
each of which is followed by a list of lookup names.

If the font contains ttf hinting, then the file may contain truetype tables,
these may be stored in several formats depending on the table. For containing
truetype instructions (fpgm, prep):

::

   TtTable: prep
   PUSHW_1
    640
   NPUSHB
    255
    251
    254
    3
    ...

The first line says that this is the 'prep' table, subsequent lines provide the
instructions of that table. These are stored in the format used by fontforge,
there's a table giving the conversion between bytecode and instruction name in
the file ttfinstrs.c (I won't introduce it here because it is rather long).

::

   ShortTable: cvt  255
     309 "Big stem width, vertical"
     184 "Small stem width, vertical"
     203 "Stem width, horizontal"
     203
    ...

The next table format is used for the ``'cvt '`` and ``'maxp'`` tables. It is
simply a list of short numbers. The first line identifies it as the cvt table
and indicates that there will be 255 numbers (note: NOT 255 *bytes*, but 2*255
bytes).

In the ``'cvt '`` table (but not in ``'maxp'``) there may be additional
comments, one for each number. These are simply descriptive comments which
remind people what each cvt entry is supposed to do. They are totally optional.

::

   TtfTable: LILY 4360
   5S;o3()It?eJ8r@H[HSJH[H^@!b&BQ*?Vcm@'XSh+1MACZ>Up/\,o1+Ca't2!<ocH+Wn2p"@,t&
   +Wo+[()Is6G8:u7D/^7,*,KO/(E=N5!=s)LCMjn,:Mp3:DSL&j05dG#cY`hLCN"!<CBO$@s(_ZX
   ...

FontForge will also store tables it doesn't understand, these will be stored in
uninterpreted binary which is packed using the ASCII85Encode [#ascii85]_ encoding
filter. The first line says that the 'LILY' table is 4360 bytes long.
Subsequent lines will provide those 4360 bytes of data ASCII85Encode [#ascii85]_. See
the PostScript Reference Manual (3rd edition, pages 131-132) for a description
of this packing, or ``$ man btoa``).

The ``LangName`` entries represent the TrueType name table: the number
represents the language and is followed by a list of strings encoded in UTF-7.
The first string corresponds to ID=0 (Copyright), the second to ID=1 (Family),
... trailing empty strings will be omitted. In the American English language
(1033) section, if one of these names exactly matches the equivalent postscript
item then that name will be omitted (this makes it easier to handle updates,
users only have to change the copyright in one place)

::

   LangName: 1033 "" "" "Regular" "GWW:Caliban Regular: Version 1.0" "" "Version 1.0"
   LangName: 1032 "" "" "+A5oDsQ09A78DvQ05A7oDrAAA"

With version 1.6 of OpenType, you are allowed to provide name table entries for
the feature names 'ss01' - 'ss20'. These look like:

::

   OtfFeatName: 'ss01'  1036 "Riable"  1033 "Risible"

Which binds feature 'ss01' to the name "Riable" in French (language 1036) and to
"Risible" in English (language 1033).

If your font has any anchor classes:

::

   AnchorClass2: "top" "Latin marks-1" "bottom" "Latin marks-1" "Anchor-2" "Latin marks-1" "Anchor-3" "Latin marks-2" "Anchor-4" "Latin marks-2" "Anchor-5" "Latin marks-2" "Anchor-6" "Latin marks-2"

There is an Anchor Class named "top" which lives in lookup subtable "Latin
marks-1". The next class is named "bottom", the next "Anchor-2" and so forth.
(Anchor class names are output in UTF7)

Contextual or contextual chaining lookups are also stored in the font header.
The are introduced by one of the keywords: "ContextPos2", "ContextSub2",
"ChainPos2", "ChainSub2" and "ReverseChain2", and are ended by "EndFPST".
Contextual chaining lookups may check previous glyphs (called backtracking),
current glyphs and lookahead glyphs, while Contextual lookups only check for a
string of current glyphs. There are four formats:

By coverage tables

::

   ChainSub2: coverage "calt-1" 0 0 0 1
    1 1 0
     Coverage: 7 uni0C40
     BCoverage: 8 glyph388
    1
     SeqLookup: 0 "high"
   EndFPST

This defines a simple context chaining substitution by coverage class. It lives
in the lookup subtable named "calt-1". There are no classes, no backtracking
classes and no lookahead classes defined (it's by coverage table). There is one
rule. (For a greater explanation of these cryptic comments see the OpenType
specs on contextual lookups).

Then follows the first rule. The first line, "1 1 0", says how many coverage
tables there are in the normal list (1), how many in the backtrack list (1) and
how many in the lookahead list (0). Then we get the one normal coverage table,
which describes a single glyph (uni0C40). Then one backtracking coverage table
which also defines one glyph (glyph388). Finally there is one sequence lookup,
at normal position 0, we should apply the substitution named '0013'.

That is to say: If we find the glyph stream "glyph388 uni0C40", then it will
match this lookup and we should apply lookup "high" (found elsewhere) to
uni0C40.

By classes

::

   ChainSub2: class "calt-1" 3 3 0 1
     Class: 52 b o v w b.high o.high v.high w.high r.alt.high r.alt
     Class: 43 a c d e f g h i j k l m n p q r s t u x y z
     BClass: 52 b o v w b.high o.high v.high w.high r.alt.high r.alt
     BClass: 43 a c d e f g h i j k l m n p q r s t u x y z
    1 1 0
     ClsList: 2
     BClsList: 1
     FClsList:
    1
     SeqLookup: 0 "high"
   EndFPST

This defines a context chaining substitution, by classes. The format of the
first line is the same as described above. Here we have three classes for the
normal match and three for the backtracking match, and one rule. The next 4
lines define the classes. As with kerning by classes, class 0 does not need to
be explicitly defined, it is implicitly defined to be "any glyph not defined in
another class". So we define class 1 to be "b,o,v,..." and class 2 to be
"a,c,d,e,...". And then we define the backtracking classes (which here happen to
be the same as the classes for the normal match, but that isn't always the
case).

The one rule says that if we get something in normal class 2 following something
in backtracking class 1 (that is, if we get something like "ba" or "oc") then
apply lookup "high"

By glyphs

::

   ChainSub2: glyph "calt-1" 0 0 0 1
    String: 1 D
    BString: 3 c b
    FString: 1 e
    1
     SeqLookup: 0 "high"
   EndFPST

Again we have one rule. That rule says that if we get the sequence of glyphs "c
b D e" then we should apply substitution "high" to glyph "D".

And finally by reverse coverage tables

::

   ChainSub2: revcov "calt-1" 0 0 0 1
    1 1 0
     Coverage: 7 uni0C40
     BCoverage: 8 glyph388
    1
     Replace: 11 uni0C40.alt
   EndFPST

Which says that when glyph388 precedes uni0C40 then uni0C40 should be replaced
by uni0C40.alt

There may be apple state machines. These are introduced by one of the keywords:
"MacIndic2", "MacContext2", "MacInsert2" and "MacKern2", and they are terminated
with "EndASM".

::

   MacContext2: "calt-1" 16384 9 5
     Class: 320 yehhamzaabovearabic beharabic teharabic theharabic jeemarabic haharabic khaharabic seenarabic sheenarabic sadarabic dadarabic taharabic zaharabic ainarabic ghainarabic feharabic qafarabic kafarabic lamarabic meemarabic noonarabic heharabic alefmaksuraarabic yeharabic peharabic tteharabic tcheharabic veharabic gafarabic
     Class: 227 noonghunnaarabic alefmaddaabovearabic alefhamzaabovearabic wawhamzaabovearabic alefhamzabelowarabic alefarabic tehmarbutaarabic dalarabic thalarabic reharabic zainarabic wawarabic ddalarabic rreharabic jeharabic yehbarreearabic
     Class: 201 shaddakasraarabic shaddakasratanarabic shaddafathaarabic shaddadammaarabic shaddadammatanarabic fathatanarabic dammatanarabic kasratanarabic fathaarabic dammaarabic kasraarabic shaddaarabic sukunarabic
     Class: 13 tatweelarabic
     Class: 17 ttehinitialarabic
    0 0 ~ ~
    0 0 ~ ~
    0 0 ~ ~
    0 0 ~ ~
    2 32768 ~ ~
    ...
    3 32768 "high" "low"
    ...
   EndASM

The state machine begins with a line defining what lookup subtable invokes it,
some mac flags, the number of classes, and number of states in the machine. The
first four states on the mac are predefined, so we start with class 4
(yehhamzaabovearabic...). Finally there will be <number of classes>*<number of
states> lines describing transitions. We begin with the transition for state 0,
class 0, then the transition for state 0, class 1, ...

Each transition contains the next state to go to, a set of flags. There may also
be other arguments depending on the type of the state machine.

.. object::MacIndic2

   This format has no additional arguments

.. object:: MacContext2

   This format potentially contains the names of two lookup substitutions. One
   to be applied to the marked glyph, one to be applied to the current glyph
   (the special substitution "~" means do nothing and is used as a place
   holder). See above.

.. object:: MacInsert2

   This format contains two glyph lists, each preceded by a number indicating
   how many bytes follow.

   ::

      2 0 0 3 a b
      0 32768 4 fi q 0

   The first line indicates that no characters should be insert at the marked
   glyph but that "a" and "b" should be insert at the current glyph. The second
   line indicates that "fi" and "q" should be insert at the marked glyph and no
   characters at the current glyph. The flags determine whether characters are
   insert before or after the glyph.

.. object:: MacKern2

   This format contains a list of kerning offsets. First is a count field saying
   how many numbers follow, then a list of numbers which adjust the kerning for
   glyphs that have previously been pushed on the kerning stack.

There may be a list of Mac Feature/Setting names

::

   MacFeat: 0 0 0
   MacName: 0 0 24 "All Typographic Features"
   MacSetting: 0
   MacName: 0 0 12 "All Features"
   MacFeat: 1 0 0
   MacName: 0 0 9 "Ligatures"
   MacSetting: 0
   MacName: 0 0 18 "Required Ligatures"
   MacSetting: 2
   MacName: 0 0 16 "Common Ligatures"
   MacFeat: 2 1 2

There may be a Grid entry near the top of the font, this specifies the splines
to be drawn in the grid layer for the font,
:ref:`see below for a description of the splineset format <sfdformat.splineset>`:

::

   Grid
   678 -168 m 5
    -40 -168 l 5
   -678 729 m 1
    1452 729 l 1
   -678 525 m 1
    1452 525 l 1
   EndSplineSet

If your font contains a 'MATH' table you will see lines like:

::

   MATH:ScriptPercentScaleDown: 80
   MATH:ScriptScriptPercentScaleDown: 60

I shall not list all the possible entries. Basically there is one for every
constant that lives in the math table. The names are the same as the names in
the (English) MATH Info dialog.

Most math constants may also specify device tables (for more on
:ref:`device tables see below <sfdformat.device-table>`):

::

   MATH:SubscriptShiftDown: 483 {12-17 1,0,0,0,1,1}


.. _sfdformat.Outline-Char-Data:

Outline Character Data
----------------------

Then for non-CID fonts you should find a line like:

::

   BeginChars: 285 253

This means that the font's encoding has room for 285 characters and that there
are a total of 253 glyphs defined (usually control characters are not defined).

Most encodings entail specific constraints on how many encoding slots must exist
and how they must be used (which glyphs where, and in what order). That is what
an encoding means. For instance, in the UnicodeBmp encoding, there must be at
least 65536 slots numbered 0 to 65535 for the Unicode characters U+0000 to
U+FFFF. Glyphs with Unicode code points in that range must be encoded in those
slots in order according to their code points. Glyphs without Unicode code
points must be encoded in additional slots numbered consecutively starting from
65536. A file with a BeginChars: line inconsistent with its encoding or
inconsistent with the number of glyphs it actually contains is not an SFD file.
FontForge may, but does not promise to, treat attempts to load invalid files as
fatal errors, or renumber or reorder glyphs to make them match the requirements
of the encoding. Consider using "Custom" or "Original" encodings if the
requirements of other encodings are not appropriate; these encodings are less
restrictive than the others.

A character looks like:

::

   StartChar: exclam
   Encoding: 33 33 3
   Width: 258
   Flags:
   HStem: 736 13<39 155>  -14 88<162 180>
   VStem: 71 84<49 396>
   DStem2: 510 435 225 423 0.568682 -0.822558<0 124.816>
   Fore
   SplineSet
   195 742 m 0
    195 738 193 736 189 736 c 0
    175 736 155 743 155 682 c 0
    155 661 130 249 130 131 c 0
    130 100 96 99 96 131 c 0
    96 149 71 662 71 682 c 0
    71 731 51 736 37 736 c 0
    33 736 31 738 31 742 c 0
    31 748 36 747 38 749 c 1
    188 749 l 1
    190 747 195 748 195 742 c 0
   80 32 m 0
    81 53 95 75 116 74 c 0
    137 73 150 53 150 32 c 0
    150 10 137 -14 115 -14 c 0
    93 -14 79 10 80 32 c 0
   EndSplineSet
   EndChar

The first line names the character. If you are using a non-standard glyph
namelist with utf8 names rather than ASCII names, the name will be UTF7 encoded
and included in quotation marks. The next line gives the encoding, first in the
current font, then in unicode, and finally the original position (GID). Then the
advance width.

Then a set of flags (there are currently five flags: "H" => the character has
been changed since last hinted, "M" the character's hints have been adjusted
manually, "W" the width has been set explicitly, "O" => the character was open
when last saved, and "I" the character's instructions are out of date).

Then horizontal and vertical (postscript) stem hints (set of several two number
pairs, the first number in the pair is the location of the stem, the next number
is the width of the stem, the numbers in brokets (<>) indicate where the hint is
valid).

Diagonal stems (DStem2) are more complex. There are 6 numbers, in 3 pairs of
two. The first pair represents a point on the left side of the hint, the second
pair a point on the right and the third pair is a unit vector in the hint
direction. Again there are numbers in brokets indicating where the hint is
valid.

For fonts with vertical metrics there may also be a

::

   VWidth: 1000

specifying the vertical advance width.

.. _sfdformat.splineset:

The entry ``Fore`` starts the foreground splines, they are encoded as postscript
commands with moveto abbreviated to m, curveto to c and lineto to l (lower case
el). The digit after after the letter is a set of flags whose bits have the
following meanings:

.. object:: 0x3

   indicates whether the point is curve (0), corner (1) or tangent (2).

.. object:: 0x4

   point selected

.. object:: 0x8

   point has default next control point

.. object:: 0x10

   point has default prev control point

.. object:: 0x20

   point is to be rounded in x (truetype hinting. doesn't really work)

.. object:: 0x40

   point is to be rounded in y (truetype hinting. doesn't really work)

.. object:: 0x80

   point was interpolated between two control points (when read from a ttf file) and so has no point number of its own

.. object:: 0x100

   point should never be interpolated

.. object:: 0x200

   Any extrema on the preceding spline are marked as acceptable to the validator


Splines for a truetype font will have two additional numbers after the flags.
These are the truetype point numbers for the point itself and for the subsequent
control point. If the value is -1 then this point has no number.

Splines for a font with hint substitution will have a hint mask after any point
before which hint substitution occurs,

::

   459 422 m 1xc0
    339 442 l 1xa0
    312 243 l 1

So the first two points have hint masks "xc0" and "xa0", these masks may be
(almost) arbitrarily long, depending on the number of hints in the glyph. "xc0"
means that the first two hints are active (0x80 & 0x40) while "xa0" means that
the first and third are (0x80 and 0x20).

If you have been using
`Raph Levien's Spiro package <http://www.levien.com/spiro/>`_ you may also have
a set of Spiro control points. These appear inside the SplineSet list after each
contour. It is quite possible that some contours will have spiros and that
others may not.

The following is the lower case 'a' glyph from Raph's
`Inconsolata font <http://www.levien.com/type/myfonts/inconsolata.html>`_,
converted into an sfd file.

::

   Fore
   SplineSet
   115 467 m 1
    134.212 486.845 157.062 503.125 182 515 c 0
    221.656 533.883 266.084 541.787 310 541 c 0
    338.088 540.496 366.236 536.384 392.804 527.254 c 0
    419.37 518.123 444.305 503.666 464.14 483.772 c 0
    483.975 463.879 498.253 438.648 505.793 411.587 c 0
    513.333 384.526 514 356.092 514 328 c 2
    514 0 l 1
    435 0 l 1
    435 58 l 1
    381.951 14.5264 314.586 -12.708 246 -13 c 0
    205.572 -13.1719 164.446 -3.24219 131.088 19.5986 c 0
    97.7295 42.4385 73.3516 78.9277 68 119 c 0
    64.5488 144.84 68.8574 171.584 79.7275 195.279 c 0
    90.5977 218.975 107.824 239.525 128.391 255.545 c 0
    169.524 287.585 222.188 301.168 274 307 c 0
    321.422 312.338 369.278 312 417 312 c 2
    434 312 l 1
    434 331 l 2
    434 346.261 434.018 361.578 431.955 376.699 c 0
    429.892 391.819 425.593 406.762 418 420 c 0
    407.035 439.119 389.166 453.909 368.792 462.316 c 0
    348.418 470.724 326.037 473.348 304 473 c 0
    272.076 472.496 240.025 466.302 211 453 c 0
    190.445 443.58 171.617 430.351 156 414 c 1
    115 467 l 1
     Spiro
       115 467 v
       182 515 o
       310 541 o
       514 328 [
       514 0 v
       435 0 v
       435 58 v
       246 -13 o
       68 119 o
       274 307 o
       417 312 [
       434 312 v
       434 331 ]
       418 420 o
       304 473 o
       211 453 o
       156 414 v
       0 0 z
     EndSpiro
   437 248 m 1
    418 248 l 2
    372.981 248 327.844 249.961 283 246 c 0
    248.938 242.992 213.941 235.036 187.152 213.785 c 0
    173.758 203.159 162.801 189.275 156.555 173.36 c 0
    150.308 157.445 148.943 139.609 153 123 c 0
    158.267 101.438 172.606 82.5566 191.107 70.2959 c 0
    209.608 58.0342 231.83 52.0508 254 51 c 0
    293.532 49.126 333.197 61.8564 366 84 c 0
    387.405 98.4502 407.011 116.318 420.258 138.489 c 0
    426.881 149.574 431.634 161.775 434.188 174.434 c 0
    436.742 187.092 437 200.087 437 213 c 2
    437 248 l 1
     Spiro
       437 248 v
       418 248 ]
       283 246 o
       153 123 o
       254 51 o
       366 84 ]
       437 213 [
       0 0 z
     EndSpiro
   EndSplineSet

The spiro data follows Raph's "Plate file" conventions. Each control point has a
location (x,y) and a point type. A point type is either:

* ``{`` -- May only be on the first control point, indicates that the contour is
  open
* ``v`` -- Indicates a corner point
* ``o`` -- Indicates a G4 curve point
* ``c`` -- Indicates a G2 curve point
* ``[`` -- Indicates a left point
* ``]`` -- Indicates a right point

The last spiro should have a point type of ``z``. It is not part of the contour,
it merely marks the end of the contour.

(Actually this doesn't quite follow Raph's conventions: His plate files have a
different coordinate system, and his final ``z`` doesn't have any coordinates).

If the glyph should open in spiro mode (displaying spiro control points rather
than bezier controls) there will be an "InSpiro" entry

::

   InSpiro: 1
   Flags: HO
   Fore

A character need not contain any splines:

::

   StartChar: semicolon
   Encoding: 59 59
   Width: 264
   Flags:
   HStem:
   VStem:
   Fore
   Refer: 33 44 N 1 0 0 1 0 0 1
   Refer: 35 46 N 1 0 0 1 0 414 2
   EndChar

Above is one with just references to other characters (a semi-colon is drawn
here by drawing a comma and stacking a period on top of it). The first number is
the glyph index of the character being referred to (in the current font of
course), the next number is the unicode code point, the N says the reference is
not selected (An "S" indicates it is selected), the following 6 numbers are a
postscript transformation matrix, the one for comma (unicode 44) is the identity
matrix, while the one for period (unicode 46) just translates it vertically 414
units. The final number is a set of truetype flags:

* 1 => Use My Metrics
* 2 => Round to Grid
* 4 => Position reference by point match (rather than by offset)

  If this is set there will be two additional numbers, the first indicating the
  point number in the base glyph, and the second the point number in the current
  reference.
* ::

     Ref: 33 44 N 1 0 0 1 0 0 1
     Ref: 35 46 N 1 0 0 1 0 414 6 3 7 O
* The bottom line indicates that point 3 and point 7 will be positioned together.
  The optional "O" is a flag which indicates that this information is out of date.

A set of splines in the background is similar, it will be introduced by a
``Back`` entry, it may also have spiros.

::

   Back
   SplineSet
   195 742 m 0
    195 738 193 736 189 736 c 0
    175 736 155 743 155 682 c 0
   ...
   Refer 33 44 N 1 0 0 1 0 0 1

While a background image is stored in the following horrible format:

::

   StartChar: A
   ...
   Back
   Image: 167 301 0 21 2 1 23 753 2.53892 2.53892
   J:N0SYd"0-qu?]szzz!!#7`s7cQozzz!!!!(s8Viozzzz"98E!zzzz!!3-"rVuouzzz!!!'"
   s8N'!zzz!!!!$s8W,7zzzz"98E$huE`WzJ+s!Dz!"],0s6p!g!!!!"s8W-!n,NFg!!!Q0s8Vio
   z5QCc`s82is!!!!`s8W,gz!WW3"s8W&uzJ,fQKp](9o!!iQ(s8W-!z!<<*!s7cQo!!",@s8W-!
   ...
   EndImage
   EndChar

Where the numbers on the image line mean respectively: width (of image in
pixels), height, image type (0=>mono, 1=>indexed, 2=>rgb (true color), 3=>rgba),
bytes per line, number of color entries in the color table, the index in the
color table of the transparent color (or for true color images the transparent
color itself), the x and y coordinates of the upper left corner of the image,
the x and y scale factors to convert image pixels into character units. Then
follows a bunch of binary data encoded using Adobe's Encode85 filter (See the
PostScript Reference manual for a description). These data contain all the
colors in the color table followed by a dump of the image pixel data.

Bitmap data will be compressed by run length encoding. I'm not going to go into
that in detail, if you want to understand it I suggest you look at the file
sfd.c and search for image2rle to see how it is done. The image is compressed
using rle and then output as above, only now there is one more parameter on the
"Image:" line which gives the number of bytes to be read from the data stream.

In multilayered fonts the foreground layers may contain images too. They are
stored in the same way.

If a glyph has extra layers beyond foreground and background they are introduced
with

::

   Layer: 2
   SplineSet
   ...

A postscript glyph may also contain countermasks:

::

   StartChar: m
   Encoding: 109 109 77
   Width: 785
   HStem: 0 18<28 224 292 488 566 752> 445 27<280 296 542 558>
   VStem: 98 56<45 376> 362 56<45 378> 626 56<45 378>
   CounterMasks: 1 38
   ...
   EndChar

The CounterMasks line in this example declares one counter group (first
argument). The "38" (and any other values following it) is a bitmask, given in
hex, that describes a group. The size of the bitmask is always a multiple of
eight (i.e. always an even number of hex digits). The highest-order bit in the
mask specifies whether the first stem hint is present (1) or absent (0) in the
counter group. The second-highest-order bit does the same for the second hint,
and so on. Any extra low-order bits not corresponding to any hint are ignored.
(I know, starting from the high bit instead of the low bit seems strange, but
that was Adobe's design decision.) Here, the third (0x20), fourth (0x10) and
fifth (0x08) stem hints (i.e. the three vertical stems) are in the group,
yielding a counter mask of 0x38. If we were to add four more VStem hints to the
glyph, making nine hints in all, then the mask would have to be given as 0x3800
(because two bytes are needed to accommodate nine bits).

Glyphs in quadratic fonts (truetype) may containing truetype instructions, these
may be output in two formats, either an text format or an old binary format:

::

   TtInstrs:
   NPUSHB
    4
    251
    0
    6
    251
   MDAP[rnd]
   MDRP[rnd,grey]
   MDRP[rp0,rnd,grey]
   ...

As with the 'prep' table above this is simply a list of truetype instruction
names as used by FontForge (see ttfinstrs.c for a table of these).

An older format is simply a binary dump in ASCII85 encoding. (Note that these
are distinguished by the initial keyword ``TtInstrs`` vs. ``Tt\ *f*Instrs``)

::

   TtfInstrs: 107
   5Xtqo&gTLA(_S)TQj!Kq"UP8<!<rr:&$QcW!"K,K&kWe?(^pl]#mUY<!s\f7"U>G:!%\s-3WRec
   $pP.r$uZOWNsl$t"H>'?EW%CM&Cer:&f3P>eEnad5<Qq=rQYuk3AE2g>q7E*
   EndTtf

This is 107 bytes of ASCII85Encode [#ascii85]_ encoded binary data.

If the character contains Anchor Points these will be included:

::

   AnchorPoint: "bottom" 780 -60 basechar 0
   AnchorPoint: "top" 803 1487 basechar 0

the point names the anchor class it belongs to (in UTF-7), its location, what
type of point it is (basechar, mark, baselig, basemark, entry, exit), and for
ligatures a number indicating which ligature component it refers to. You may
also see:

::

   AnchorPoint: "bottom" 780 -60 basechar 0 {12-13 -1,-1} {8-14 1,0,-1,-1,-2,-2,-2}

.. _sfdformat.device-table:

Where the items in braces are horizontal/vertical device tables. The first
indicates that the (horizontal) device table applies to pixel sizes 12 through
13 with pixel adjustments of -1 pixel each. The second indicates a vertical
device table applies to pixel sizes 8 through 14 with pixel adjustments of 1, 0,
-1, -1, -2, -2, -2 pixels.

Finally a TrueType font may position an anchor point based on a normal point
within the glyph (if this is done a device table may not be present).

::

   AnchorPoint: "bottom" 780 -60 basechar 0 23

Indicates that this anchor point will be positioned at the same location (which
is normally 780,-60 but which might be moved by an instruction) as truetype
point 23.

If the user has set the glyph class

::

   GlyphClass: 2

Where the number is 1 more than the 'GDEF' glyph class it represents. (so the 2
above means a base glyph (class 1)).

If a glyph has multiple unicode encodings (the glyph for latin "A" might be used
for greek "Alpha"), or is specified by variation selector then alternative
unicode information will be provided:

::

   AltUni2: 000061.00fe01.0

Where the first number in the dotted triple is the alternate unicode code point
(in hex), the next number is the variation selector (or ffffffff), and the last
number is reserved for future use (and must currently be 0). There may be more
than one triple on the line.

If the character is the first in any kern pairs (not a pair defined by a kern
class, however)

::

   Kerns2: 60 -100 "Kern Latin" {12-13 -1,-1} 63 -92 "Kern Latin" 70 -123 "Kern Latin" 45 -107 "Kern Latin" 76 -107 "Kern Latin"

Where each kern pair is represented by 2 numbers and a lookup subtable (and an
optional :ref:`device <sfdformat.device-table>` table, see above). The first is
the original position of the second character (in the current font), the next is
the horizontal kerning amount, then the lookup subtable name. Then we start over
with the next kernpair.

Data that are to go into other GPOS, GSUB or GDEF sub-tables are stored like
this:

::

   Position2: "Inferiors" dx=0 dy=-900 dh=0 dv=0
   PairPos2: "Distances" B dx=0 dy=0 dh=0 dv=0  dx=-10 dy=0 dh=0 dv=0
   Ligature2: "Latin Ligatures" one slash four
   Substitution2: "Latin Smallcaps" agrave.sc
   AlternateSubs2: "Latin Swash" glyph490 A.swash
   MultipleSubs2: "Latin Decomposition" a grave
   LCarets2: 1 650

In most of these lines the first string is the lookup subtable name (except for
LCarets where there is nothing). A simple position change is expressed by the
amount of movement of the glyph and of the glyph's advance width. A pairwise
positioning controls the positioning of two adjacent glyphs (kerning is a
special case of this). A ligature contains the names of the characters that make
it up. A simple substitution contains the name of the character that it will
become. An alternate sub contains the list of characters that the user may
choose from. A multiple substitution contains the characters the current glyph
is to be decomposed into. A ligature caret contains a count of the number of
carets defined, and a list of the locations of those carets.

Some glyphs may have python data:

::

   PickledData: "I3
   ."

This is arbetrary python pickled data (protocol=0) which got set by a python
script. FontForge stores it as a string. If there are either double quotes or
backslashes inside the string they will be preceded by a backslash.

A glyph may also have

::

   Comment: Hi
   Colour: ff0000
   Validated: 1
   UnlinkRmOvrlpSave: 1

The first is an arbitrary comment, this will be output in UTF-7, the second a
color (output as a 6 hex digit RGB value), the cached validation state (a
bitmask, see the python docs for the bits' meanings, there can be one of these
for every layer), and a flag that the glyph should have its references unlinked
and remove overlap run before the font is saved.


TeX & MATH data
^^^^^^^^^^^^^^^

A line like

::

   TEX: 0 425

Specifies the tfm height and depth of a glyph.

::

   ItalicCorrection: 50
   TopAccentHorizontal: 400
   IsExtendedShape: 1
   GlyphVariantsVertical: parenleft.big parenleft.bigger parenleft.biggest
   GlyphConstructionVertical: 3  uni239D%0,0,300,4733 uni239C%1,2500,2500,2501 uni239B%0,300,0,4733
   TopRightVertex: 2 0,0{13-15 1,0,1} 100,10

Specifies the Italic Correction (either from a tfm file or the MATH table).
Italic Correction may also include a device table.

Specifies the horizontal placement of top accents in mathematical typesetting.
This may also include a device table.

Specifies the current glyph is an extended shape (and therefore may need special
superscript positioning).

For both glyph variants and construction the word "Vertical" may be replaced
with Horizontal for glyphs that grow horizontally.

The first number in GlyphConstruction is the number of components. Each
component is represented by a glyphname, followed by "%", followed by an
indication of whether this is an extender, followed by the start overlap length,
the end overlap length and the full length.

Math Kerning info may be specified for each vertex of the glyph (TopRight,
TopLeft, BottomRight, BottomLeft). First is a count of the number of points,
then the height, kerning pairs for each. Device tables are permitted and may
follow directly after either the height or the kerning value.


Outline character extensions for multilayered fonts (type3)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Instead of having a single "Fore"ground layer, multilayered type3 fonts have
(potentially) several layers, each one introduced by a Layer line which
specifies things like filling and stroking info for this layer. Then within each
layer you may find a splineset, a list of references and a list of images (any
or all may be omitted). The syntax for these is the same as in the normal case
above.

::

   LayerCount: 3
   Layer: 1  1 1 1  #fffffffe 1  #fffffffe 1 40 round round [1 0 0 1] []
   FillGradient: 220;260 490;450 0 pad 2 {0 #808080 1} {1 #000000 1}
   SplineSet
   ...
   EndSplineSet
   Layer: 2  1 0 1  #00ff00 1  #0000ff 1 19 miter butt [0.5 0 0 1] [10 5]
   FillGradient: 400;400 400;400 400 repeat 2 {0 #ffffff 1} {1 #808080 1}
   SplineSet
   ...
   EndSplineSet
   Layer: 3  1 0 1  #00ff00 1  #0000ff 1 19 miter butt [0.5 0 0 1] [10 5]
   FillPattern: E 200;200 [0.707 0.707 -0.707 0.707 0 0]
   SplineSet
   ...
   EndSplineSet

The LayerCount line gives the number of layers in the glyph (the background
layer is layer 0, so this number is one more than the number of layers that will
actually be specified -- the background layer is still specified the way it was
before, but is included in this count). This line is not required, if omitted
(or wrong) FontForge will figure that out.

The Layer line contains too much information. First is a number saying which
layer, next are three booleans, the first specifies whether to fill the
splineset (or use an imagemask for images), the second whether to stroke the
splineset, the third is currently pretty much meaningless. Next follows the RGB
value of the fill color (the special value of #fffffffe means the color is
inherited), then the opacity (the special value -1 means inherited), then the
stroke color and opacity. Then the stroke width (again -1 means inherited), the
linejoin type (miter, round, bevel, inherited), the linecap type (butt,
round,square,inherited), the transformation matrix to be applied to the pen and
finally the dash array.

Any SplineSet, Ref, Images between this Layer and the next (or the end of the
character) are part of this layer.

The FillGradient lines allow you to specify linear or radial gradient fills (or
StrokeGradient lets you specify a gradient for the strokes). It is followed by
two points, the start and end points of a linear gradient, the focus and center
of a radial gradient. Then a radius (if this is 0 then we have a linear
gradient, otherwise a radial one). A keyword which specifies how the gradient
should behave outside the region specified by the start and end points. Finally
there is a number which gives the number of "stop-points". Each stop point is in
curly braces and contains three numbers, one between 0 and 1 (indicating the
location from the start of the gradient to it's end), one a hex colour, and one
the opacity, also between 0 and 1.

The FillPattern line allows you to specify a pattern tiled instead of more
traditional single colour fills (or StrokePattern lets you specify a pattern for
the strokes). It is followed by the name of a glyph (which contains the
pattern), the size of the pattern (width, height) in the current coordinate
system (so the pattern glyph will be scaled until it is this size). And a
transformation matrix to be applied after the pattern has been tiled across the
plain (the one here rotates the tiles by 45 degrees).

If a glyph is used as a tile it may have one of the following additional
keywords:

::

   TileMargin: 20
   TileBounds: -20 -220 1020 820

If neither of these is specified the tile's size will be its bounding box. If
TileBounds is specified, the size is taken from the box information specified
there (minx, miny, maxx, maxy). If TileMargin is specified then the bounding box
will be extended by this amount (to provide a whitespace margin around the
tile).

Inside a SplineSet a contour may be modified by a PathFlags keyword (output
after the contour has been completed). Currently there is only one flag (1)
which indicates the that contour is part of the cliping path of the glyph.

::

   SplineSet
   102 699 m 5
    1176 699 l 5
    1176 -120 l 5
    102 -120 l 5
    102 699 l 5
     PathFlags: 1
   EndSplineSet


.. _sfdformat.Bitmap-Fonts:

Bitmap Fonts
------------

After all the outline characters have been described there is an EndChars entry
and then follow any bitmap fonts:

::

   EndChars
   BitmapFont: 12 285 10 2 1
   BDFStartProperties: 2
   COMMENT 0 "This is a bdf comment property"
   FONT_DESCENT 18 2
   BDFEndProperties
   Resolution: 75
   BDFChar: 32 3 0 0 0 0
   z
   BDFChar: 3 33 3 0 1 0 9
   ^d(.M5X7S"!'gMa
   BDFRefChar: 302 488 7 1 N
   BDFRefChar: 302 58 0 0 N

The bitmap font line contains the following numbers: the pixelsize of the font,
the number of potential characters in the font, the ascent and the descent of
the font and the depth of font (number of bits in a pixel).

Optionally there may be a set of BDF properties. If there are any properties
there will be a line "BDFStartProperties:" with a count of properties. There
should be that many property lines, and then a "BDFEndProperties" line. Each
property is of the form:

``<NAME> <TYPE> <VALUE>``

Where <NAME> is the property name, <TYPE> is 0 for a string, 1 for an atom, 2
for an int, 3 for an unsigned int. In addition <TYPE> may have a 16 ored to one
of the above values indicating the line is a true property (rather than
information from somewhere else in the BDF header). <VALUE> will either be a
quoted string or an integer.

Optionally there may be a resolution line, which specifies the design resolution
of the font.

This is followed by a list of bitmap characters, the bitmap character line
contains the following numbers: the original position (glyph ID), the encoding
(local), the width, the minimum x value, the minimum y value, the maximum x
value and the maximum y value. If the bitmap font has vertical metrics there
will also be a vwidth. This is followed by another set of binary data encoded as
above. There will be (ymax-ymin+1)* ((xmax-xmin)/(8/depth)+1) (unencoded) bytes,
there is no color table here (the high order bit comes first in the image, set
bits should be colored black, clear bits transparent).

If there are any bitmap glyphs with bitmap references then these will appear
near the end of the bitmap strike. Each ``BDFRefChar`` line defines a reference,
the first number is the glyph ID of the composite glyph, the second the glyph ID
of the referred glyph, and the last two the horizontal and vertical translation
of the reference in the composite glyph.

A bitmap font is ended by:

::

   EndBitmapFont
   BitmapFont: 17 285 14 3 1
   BDFChar: 0 17 0 0 0 0
   z
   ...
   EndBitmapFont
   EndSplineFont


.. _sfdformat.CID-keyed-fonts:

CID keyed fonts
^^^^^^^^^^^^^^^

A CID font is saved slightly differently. It begins with the normal font header
which contains the information in the top level CID font dictionary. As
mentioned above this will include special keys that specify the CID charset
(registry, ordering, supplement). It will also include:

::

   CIDVersion: 2.0
   BeginSubFonts: 5 8318

The ``CIDVersion`` is self-explanatory. The ``BeginSubFonts`` line says that
there are 5 subfonts the largest of which contains slots for 8318 characters
(again some of these may not be defined). This will be followed by a list of the
subfonts (dumped out just like normal fonts) and their characters. Only the top
level font will contain any bitmap characters, anchor classes, etc.


.. _sfdformat.Multiple-Master-fonts:

Multiple Master fonts
^^^^^^^^^^^^^^^^^^^^^

Multiple master fonts start with a different style of file header, and are
followed by a set of sub fonts. If the mm font has 4 instances then there will
be 5 subfonts (one for each instance, and one for the blended font). The font
header looks like:

::

   SplineFontDB: 1.0
   MMCounts: 4 2 0 0
   MMAxis: Weight Width
   MMPositions: 0 0 1 0 0 1 1 1
   MMWeights: 0.31502 0.13499 0.38499 0.16499
   MMAxisMap: 0 2 0=>50 1=>1450
   MMAxisMap: 1 2 0=>50 1=>1450
   MMCDV:
   {
   1 index 1 2 index sub mul 3 1 roll
   1 2 index sub 1 index mul 3 1 roll
   1 index 1 index mul 3 1 roll
   pop pop
   0 1 1 3 {index add} for 1 exch sub 4 1 roll
   }
   EndMMSubroutine
   MMNDV:
   {
   exch 50 sub 1400 div
   exch 50 sub 1400 div
   }
   EndMMSubroutine
   BeginMMFonts: 5 0

The "MMCounts" line gives the number of instances (4) and the number of axes (2)
in this font also whether it is an apple distortable font (0) and if so the
number of named styles (0) in that font. The "MMAxis" line gives the names of
the axes. The MMPositions line is an array of real numbers (with
instance_count*axis_count elements) describing the coordinates of each instance
along each axis. The MMWeights line provides the weights (blends) of the
instance fonts to which, when interpolated, yield the default font. There will
be one AxisMap line for each axis, it provides the mapping from design space to
normalized space. The first line

::

   MMAxisMap: 0 2 0=>50 1=>1450

says that axis 0 has two mapping points. One is at normalized position 0 and
corresponds to design position 50, the other is at normalized position 1 and
corresponds to design position 1450. There are two subroutines stored here, both
are simple postscript. The first is the /NormalizeDesignVector routine and the
second is the /ConvertDesignVector routine.

Finally we have the actual instance fonts.

--------------------------------------------------------------------------------

Fonts with information for Apple's '\*var' tables ('fvar', 'gvar', 'avar' and
'cvar') have a slightly different format:

::

   MMCounts: 8 2 1 10
   MMAxis: Weight Width
   MMPositions: 1 0 -1 0 0 1 0 -1 -1 -1 1 -1 1 1 -1 1
   MMWeights: 0 0 0 0 0 0 0 0
   MMAxisMap: 0 3 -1=>0.479996 0=>1 1=>3.2
   MacName: 0 0 6 "Weight"
   MacName: 0 1 15 "Type de graisse"
   ...
   MMAxisMap: 1 3 -1=>0.619995 0=>1 1=>1.3
   MacName: 0 0 5 "Width"
   MacName: 0 1 7 "Largeur"
   ...
   MMNamedInstance: 0  3.2 1
   MacName: 0 0 5 "Black"
   MacName: 0 1 9 "Tr\217s gras"
   ...

In Adobe's format coordinates range between [0,1], while in Apple's format they
range [-1,1]. Adobe generally specifies 2 instances per axis (at the extrema)
while Apple expects 3 (the extrema and the default setting at 0). So a two axis
font for Adobe will normally contain 4 instances and a default version, while
one for Apple will contain 8 instances and a default version. The MMWeights
field is irrelevant for Apple. Each axis has a set of names, for that axis
translated into various languages. Finally Apple allows certain points in design
space to be given names, here the point Weight=3.2, Width=1 is named "Black" or
"Trés gras".


.. _sfdformat.sfdir:

SplineFont Directories
----------------------

In late 2006 some people wanted a font format where each glyph was stored in a
single file (to give version control systems a finer granularity and reduce the
amount of stuff to download after changes). I have extended the format slightly
by creating what I call SplineFont Directories. These are basically sfd files
split up into little bits in directories with an extension of ".sfdir". The
directory contains the following:

* A file ``font.props`` which contains the
  :ref:`font header <sfdformat.Font-Header>` and includes everything up to (but
  not including) the ``BeginChars`` line
* For non-CID-keyed fonts the directory will contain one file for each glyph in
  the font, these files will be named ``\ *<glyph-name>*.glyph`` and will be in
  the format specified for :ref:`outline data <sfdformat.Outline-Char-Data>`.
* If the font contains any bitmap strikes then there will be subdirectories named
  ``\ *<pixel-size>*.strike``. And these directories will contain a
  ``strike.props`` file and one file per glyph in the strike, these files named
  ``\ * <glyph-name>*.bitmap``. The ``strike.props`` file will contain the bitmap
  header and bdf properties, while the ``*.bitmap`` files will contain the
  per-glyph bitmap data. These will be in the format described in the section on
  :ref:`bitmaps <sfdformat.Bitmap-Fonts>`.
* For CID-keyed fonts there will be subdirectories named
  ``\ *<subfontname>*.subfont``, each subfont will contain its own ``font.props``
  file and its own set of glyph files.
* For Multiple Master fonts there will be subdirectories named
  ``\ *<instancename>*.instance``


.. _sfdformat.Autosave-Format:

Autosave Format
---------------

:doc:`Error recovery </appendices/errrecovery>` files are saved in ~/.FontForge/autosave,
they have quite random looking names and end in .asfd. They look very similar to
.sfd files above.

If an asfd file starts with a line:

::

   Base: /home/gww/myfonts/fontforge/Ambrosia.sfd

Then it is assumed to be a list of changes applied to that file (which may be an
sfd file or a font file). If it does not start with a "\ ``Base:``" line then it
is assumed to be a new font. The next line contains the encoding, as above. The
next line is a ``BeginChars`` line. The number given on the line is not the
number of characters in the file, but is the maximum number that could appear in
the font. Then follows a list of all changed characters in the font (in the
format described above).

Bitmaps are not preserved. Grid changes are not preserved.


.. rubric:: Footnotes

.. [#sfdext]

   **The SFD extension**

   Yes, I know it stands for sub-file directory on the dear old PDP-10s.

   But they aren't really around any more, so I'll reuse it. And probably nobody
   else remembers them...

   Oh, it's also used by TeX for Sub Font Definition files (see ttf2afm).

.. [#ascii85]

   **ASCII85Encode**

   An encoding which takes 4 binary bytes and converts them
   to 5 ASCII characters between '!' and 'u'. Suppose we have 4 binary bytes: (b1
   b2 b3 b4) then we want to find (c1 c2 c3 c4 c5) such that:

   ::

      b1 * 2563  +  b2 * 2562  +  b3 * 256  +  b4 =
         c1 * 854  +  c2 * 853  +  c3 * 852  +  c4 * 85  +  c5

   The final output is obtained by adding 33 ('!') to each of the c\ :sub:`i`. If
   all four bytes are 0, then rather than writing '!!!!!', the letter 'z' may be
   used instead.

   Unfortunately, not all streams of binary data have a length divisible by 4. So
   at the end of the run, assume there are *n* bytes left over. Append *4-n* zero
   bytes to the data stream, and apply the above transformation (without the 'z'
   special case for all zeros), and output *n+1* of the c\ :sub:`i` bytes.

   So to encode a single 0 byte we would:

   * append 3 additional 0 bytes (n==1 => add 4-1=3 bytes)
   * find that all the c\ :sub:`i` were also zero
   * add '!' to each (all are now '!')
   * output two '!' (n+1 = 2)
