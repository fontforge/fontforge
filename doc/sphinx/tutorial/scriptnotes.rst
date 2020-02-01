Special thoughts for special scripts
====================================


Special thoughts for special scripts
------------------------------------

`Microsoft <http://www.microsoft.com/typography/specs/default.htm>`__ provides
some information on what (opentype) features a word processor should support by
default for certain scripts.

.. warning:: 

   Just because a feature is documented and looks useful does not mean Uniscribe
   will use it for your script. Many Latin script fonts would like to use
   'init', 'medi', 'calt', etc. but none of these features is turned on by
   Uniscribe for Latin.

.. warning:: 

   Just because Uniscribe supports a feature that does not mean any given
   application will. Uniscribe (as of 2005) supports 'liga' for latin, but
   neither Word nor Office does.

.. warning:: 

   Uniscribe (MS unicode text layout routines) may ignore either the GPOS or the
   GSUB table depending on the script, and may even refuse to use the font at
   all if it doesn't have the right stuff in GPOS/GSUB. A Hebrew font must have
   both a GPOS and a GSUB. If it doesn't the font is not used. A latin font need
   not have either, but if it doesn't have GSUB then GPOS won't be used. So now
   if one table is present and the other isn't, FontForge will generate a dummy
   version of the other.


Common
^^^^^^

Many characters are used in more than one script. The digits, marks of
punctuation, etc. are said in Unicode to belong to the script "Common". OpenType
does not recognize this script. The closest thing it has is the script 'DFLT'.
My understanding is that characters in the common script will have the script of
adjacent text assigned to them by OpenType.

Thus if a font supports latin, greek or cyrillic then digits and punctuation
might find themselves in any of those three scripts and all features which apply
to any such character should be present in all scripts. For example if the digit
9 kerns with digit 1, then that kerning data should be present in cyrillic and
greek as well as latin.

However it is possible (and I gather common in Japanese) to use the digits from
one font surrounded by Kanji characters from another font. This means the digits
may be in a font which does not support Kanji. However OpenType will assign them
the Kanji script. Thus no lookups will be applied. Adobe suggests that for most
features in scripts should also appear in the fallback script 'DFLT'. I'm not
sure that anyone else follows this convention.


Latin
^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/OpenType%20Dev/standard/shaping.mspx>`__

There are not many special complications in latin. Latin fonts can generally fit
in a single byte encoding with no (or few) font tables. There are a plethora of
accented glyphs which could be built -- or you could use mark to base
positioning. Kerning should be generated for some glyph combinations. A few
ligatures need to be generated (the "f" ligatures: ff, fi, fl, ffi, ffl and
perhaps st -- however for some languages (Turkish) the "fi" ligature should not
be built).

You may want to add a set of smallcaps. Adobe has reserved a block in the
private use area for latin small-caps -- this is now depreciated.

Some languages have specific requirements of their own

* `For Polish <http://studweb.euv-frankfurt-o.de/twardoch/f/en/typo/ogonek/kreska.html>`__


Greek
^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/OpenType%20Dev/standard/shaping.mspx>`__

Greek also does not have many complications. Modern Greek fonts generally fit in
a single byte encoding. For modern greek are a few accented glyphs that need to
be built, while for polytonic greek there are many -- mark to base & mark to
mark are options. Kerning should be generated. I am not aware of any standard
ligatures for modern greek (ancient greek had ligatures and variants on some of
the glyphs though).

Small caps are again an option, and I have reserved a block in the private use
area for them -- again this is depreciated.


Cyrillic
^^^^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/OpenType%20Dev/standard/shaping.mspx>`__

Cyrillic fonts also fit in a single byte encoding. There are a few accented
glyphs. Kerning should be generated. I am not aware of any standard ligatures.

Some languages need variant glyphs (specified with a 'loca' feature)

* `Serbian/Macedonian <http://jankojs.tripod.com/SerbianCyr.htm>`__


Arabic
^^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/OpenType%20Dev/arabic/shaping.mspx>`__

Arabic needs a complete set of initial, medial, final and isolated forms --
Unicode has reserved space for these. Arabic also needs a vast set of ligatures
-- Unicode has reserved space for many, but I'd guess that extra ligatures will
be needed sometimes. Arabic also needs a set of mark (mark to base, mark to
ligature) attachments to position vowels above letters. Arabic may need a glyph
decomposition table.

I'm told that in good Arabic typography there need to be many more than 4 forms
per glyph. I'm not sure how this should be supported.

Right to left.


Hebrew
^^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/OpenType%20Dev/hebrew/shaping.mspx>`__

Hebrew has a few final forms but no special tables are needed for these. Hebrew
may need kerning. Hebrew should have a set of mark (mark to base) tables to
position vowels over letters. Hebrew may need a glyph decomposition table. I am
not aware of any needed ligatures.

Right to left


Indic scripts
^^^^^^^^^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/otfntdev/indicot/shaping.aspx>`__

Indic scripts need a set of ligatures.

(they probably need a lot more but I'm not aware of what)


Korean Hangul
^^^^^^^^^^^^^

Uniscribe supports the
`following features <http://www.microsoft.com/typography/OpenType%20Dev/hangul/shaping.mspx>`__

The Hangul script consists of a set of syllables built out of a phonetic
alphabet. Generally fonts consist of a set of precomposed syllables.

Complications are introduced by the massive combinatorial explosion of all these
syllables. These are eased in postscript by CID-keyed fonts.

Vertical writing and left to right writing are used, and some glyphs have a
different orientation when drawn vertically (parentheses for example).


Japanese and Chinese (and Korean Hanja)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

MicroSoft does not describe these scripts (that I can find).

Again a massive collection of glyphs is needed, and postscript uses CID keyed
fonts to deal with this.

Vertical writing and left to right writing are used, and some glyphs have a
different orientation when drawn vertically (parentheses for example).