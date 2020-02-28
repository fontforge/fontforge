Encoding Menu
=============

The Encoding menu is only found in the font view.

.. _encodingmenu.Reencode:

.. object:: Reencode

   Has an attached sub-menu of standard and user defined encodings. The font's
   current encoding will be indicated with a check mark. You may change the
   encoding by selecting a different entry.

   There are two slightly different formats an encoding can take. It can be
   defined by unicode code points, or it can be defined by glyph names. When
   reencoding to an encoding defined by code points, the glyph with the matching
   unicode value is placed in the encoding slot. When reencoding to an encoding
   defined by glyph name, we first search for a glyph with the matching name and
   use it, if not found we search for the glyph name's corresponding unicode
   code point (if any) and if found we change its name to that specified by the
   encoding.

   Example: Suppose we have a font containing a glyph named "uni0041", and an
   encoding which maps U+0041->slot 65, then the glyph will be moved into slot
   65. If we have another encoding which maps "A" -> slot 65, then (since "A"
   has unicode value U+0041) our glyph will still be mapped to slot 65, but in
   addition its name will be changed to "A".

.. _encodingmenu.Compact:

.. object:: Compact

   Remove any holes from the encoding so all the glyphs get smushed together. If
   the font is already compact, then selecting this again will restore the
   original.

.. _encodingmenu.Force:

.. object:: Force Encoding

   Has the same sub-menu as above. Here we assume that the glyphs of the font
   are currently encoded in the right order, but they have the wrong names (This
   may seem odd, but it happens a lot). This command will change the names of
   all the glyphs to match what they should be if the indicated encoding were in
   force.

.. _encodingmenu.AddSlots:

.. object:: Add Encoding Slots...

   Add some extra slots at the end of the font into which you can put unencoded
   glyphs (variant glyphs, etc.)

.. _encodingmenu.RemoveSlots:

.. object:: Remove Unused Slots

   Removes any unused slots from the end of the font. It does not remove unused
   slots inside the font, that would screw up the encoding.

.. _encodingmenu.Detach:

.. object:: Detach Glyphs

   Detaches any selected encoding slots from their currently associated glyphs.
   These slots will now be marked as unused. The glyphs will remain in the font,
   just not encoded (If you reencode the font those glyphs will become visible
   again).

.. _encodingmenu.RemoveGlyphs:

.. object:: Detach & Remove Glyphs...

   Similar to the above except that any glyphs detached (which are not used
   elsewhere in the encoding) will be removed from the font.

.. _encodingmenu.AddEnc:

.. object:: Add Encoding Name...

   Requests an encoding name from the user and searches for it in the iconv()
   database. It then adds that encoding to the menu.

.. _encodingmenu.Load:

.. object:: Load Encoding...

   Asks the user for a filename and attempts to load a user defined encoding
   from that file. (You can only load small encodings -- one byte encodings)

.. _encodingmenu.Make:

.. object:: Make from Font...

   Allows you to name the font's current encoding (if it isn't already named),
   and add it to the encoding menu.

.. _encodingmenu.RemoveEnc:

.. object:: Remove Encoding...

   Removes one of the user defined encodings from the menu.

.. _encodingmenu.Display:

.. object:: Display by Group...

   Allows you restrict the glyphs displayed in the font view to those in a user
   defined :doc:`group </ui/dialogs/groups>` (specified in the next command).

.. _encodingmenu.Define:

.. object:: Define Groups...

   Allows you to define :doc:`groups </ui/dialogs/groups>` of glyphs which (presumably) have
   some meaningful connection to each other.

.. _encodingmenu.SaveNL:

.. object:: Save NameList of Font...

   Creates a :ref:`namelist <encodingmenu.namelist>` file mapping unicode to the
   glyph names of the current font.

.. _encodingmenu.LoadNL:

.. object:: Load NameList...

   Loads a :ref:`namelist <encodingmenu.namelist>` file into fontforge and
   copies it so that fontforge will load it on start up in the future.

.. _encodingmenu.RenameGlyphs:

.. object:: Rename Glyphs...

   Allows you to specify a :ref:`namelist <encodingmenu.namelist>`. All glyphs
   in the current font will be renamed to match the scheme in the namelist.

.. _encodingmenu.NameGlyphs:

.. object:: Create Named Glyphs...

   Allows you to specify a file containing a list of glyph names. FontForge will
   create a sequence of unencoded glyphs with these names. This might be useful
   if all your fonts contain small caps and you always want to have the names
   "A.small", "B.small", "C.small" etc. or perhaps you always want the ligatures
   "longs_longs_t", "f_longs", "f_j", etc.


.. _encodingmenu.General-encodings:

General notes on encodings
--------------------------

Not all font formats support all encodings. SVG fonts will always be output in a
unicode encoding, truetype fonts in either unicode or one of the CJK encodings,
type1 fonts only support single byte encodings, etc.

In a CID keyed font you are not allowed to change the encoding (in essence
because there is none), but there is an entry
:ref:`CID->Change Supplement <cidmenu.ChangeSup>` which will display the
Registry/Ordering information and allow you to change the supplement.

--------------------------------------------------------------------------------


.. _encodingmenu.Encodings:

Built in Encodings
^^^^^^^^^^^^^^^^^^

FontForge knows about the following encodings by default:

* ISO-8859-1 (Latin1) -- traditional encoding for western european characters.
  Default encoding for http. Does not include the Euro sign.
* ISO-8859-15 (Latin0) -- Replacement for Latin1. Does include the Euro.
* ISO-8859-2 (Latin2) -- Central & Eastern European (Czech, Hungarian, Polish,
  Romanian, Croatian, Slovak, Slovnian.
* ISO-8859-3 (Latin3) -- Southern European (Esperanto, Maltese)
* ISO-8859-4 (Latin4) -- Northern European (Estonian, Latvian, Lithuanian,
  Greenlandic, Lappish)
* ISO-8859-9 (Latin5) -- Turkish
* ISO-8859-10 (Latin6) -- Nordic (reworking of Latin4&Latin1)
* ISO-8859-13 (Latin7) -- Another Baltic character set
* ISO-8859-14 (Latin8) -- Celtic (Gaelic & Welsh)
* ISO-8859-5 (Cyrillic)
* ISO-8859-6 (Arabic)
* ISO-8859-7 (Greek)
* ISO-8859-8 (Hebrew) -- (and Yiddish)
* ISO-8859-11 (Thai) -- Also know as TIS 620

  <there is no ISO-8859-12>

--------------------------------------------------------------------------------

* KOI8-R -- Cyrillic
* Macintosh Roman
* Windows "ANSI" (CodePage1252)
* Adobe Standard
* Symbol
* TeX Base

--------------------------------------------------------------------------------

* ISO-10646-1 (Unicode, BMP)
* ISO-10646-1 (Unicode, Full)
* ISO-10646-? (Unicode, by plane)

  (You can select a specific plane of unicode as an encoding (ie BMP, SMP,
  SIP,...)

--------------------------------------------------------------------------------

* SJIS
* JIS 208 -- Japanese Kanji (first 8000 characters)
* JIS 212 -- Japanese Kanji (next 8000 characters)
* Wansung
* KSC 5601 -- Korean (this is the 94x94 version of KSC 5601)
* Johab
* GB 2312 -- Simplified Chinese
* Packed GB 2312 -- (I don't know what the proper name for this is, ASCII for
  bytes<0x80, and GB 2312 EUC offset by 0x8080)
* Big5 -- Traditional Chinese

--------------------------------------------------------------------------------

* Custom -- An unknown encoding
* Glyph Order -- the glyph ordering used in the original font file.

Encoding sources:

* `ISO 8859 Alphabet Soup <http://czyborra.com/charsets/iso8859.html>`__
* `Unicode Mapping Tables <http://www.unicode.org/Public/MAPPINGS/>`__

`An index to images of all the glyphs in unicode <http://www.unicode.org/charts/>`__.

--------------------------------------------------------------------------------


.. _encodingmenu.User-def:

User Defined Encodings
^^^^^^^^^^^^^^^^^^^^^^

You can also add new encodings to the set that FontForge knows about. There are
three menu items that manipulate a set of user defined encodings. As always
these specify both a character set and an encoding. The encoding has a maximum
of 256 entries, but the character set may be larger (up to 1024). This means
that you can define a font with extra characters. Since postscript fonts can be
reencoded at runtime this can be useful.

The Load Encoding command allows you to load an encoding(s) from a file.
Currently the file must either be in the format used by the unicode consortium
for `mapping ISO 8859 <http://www.unicode.org/Public/MAPPINGS/ISO8859/>`__
encodings to unicode, or it must be a postscript encoding array. The first
format looks like this:

::

   0x20   0x0020  #       SPACE
   0x21    0x0021  #       EXCLAMATION MARK
   ...

A postscript file looks like:

::

   /TeXBase1Encoding [
    % 00
    /.notdef /dotaccent /fi /fl
    /fraction /hungarianumlaut /Lslash /lslash
    ...
   ] def

There may be more than one encoding in a postscript file. The encoding parser is
not smart. It will only read arrays specified like this, don't try any of the
innumerable other ways of specifying an array in postscript.

If the font has a custom encoding then the ``Make From Font`` menu item is
enabled. This allows you to name the encoding you have defined for the current
font.

The ``Remove Encoding`` menu item brings up a list showing all the custom
encodings and allows you to delete them.

Here's an `example of a postscript encoding file <https://fontforge.org/downloads/Encodings.ps.gz>`__.
It contains:

* TeXMathItalicEncoding
* TeXMathSymbolEncoding
* TeXMathExtensionEncoding

--------------------------------------------------------------------------------

* IsoLatin -- (which specifies all the characters used in any of the ISO-Latin-*
  fonts

--------------------------------------------------------------------------------

* AdobeExpert -- (Which contains things like lower case numbers, small caps,
  fractions, sub/superscript numbers, etc.)

--------------------------------------------------------------------------------

* CodePage1250 -- Microsoft's encoding for Central European characters
* CodePage1251 -- Microsoft's Cyrillic encoding
* CodePage1252 -- Microsoft's Western European encoding (a superset of Latin1.
  Sometimes called "ANSI" though I can find no ANSI standard that it follows)
* CodePage1253 -- Microsoft's Greek encoding
* CodePage1254 -- Microsoft's Turkish encoding
* CodePage1255 -- Microsoft's Hebrew encoding (an extension of ISO-8859-8)
* CodePage1256 -- Microsoft's Arabic encoding
* CodePage1257 -- Microsoft's Baltic encoding
* CodePage1258 -- Microsoft's Viet Namese encoding
* CodePage874 -- Microsoft's Thai encoding

--------------------------------------------------------------------------------

* MacCentralEuropean
* MacCyrillic
* MacGreek
* MacHebrew

--------------------------------------------------------------------------------

* US-ASCII -- Not really useful by itself any more, but provides the first 128
  characters of almost every other encoding.


.. _encodingmenu.namelist:

NameLists
---------

Adobe has established a standard glyph naming convention which provides
intelligible names for many glyphs of unicode characters. And some
unintelligible names too.

A :ref:`namelist <overview.Glyph-names>` is just a mapping from unicode to glyph
names (a glyph name must be made up of alphanumeric characters (or the special
characters '.' or '_'), it may not begin with a digit, and it must be 31 or
fewer characters in length.

FontForge provides a series of standard namelists:

* Adobe Glyph List

  This is the set of names that Adobe publishes on the web.
* AGL For New Fonts

  This is set of the names that Adobe recommends for the new fonts, without odd
  names like "afiiXXXXX" or incorrect commaaccent names.
* AGL without afii

  The cyrillic and hebrew glyphs have been assigned some very odd names
  (afiiXXXXX) and some people prefer not to use them. This is now redundant; AGL
  for New Fonts should be used instead.
* AGL with PUA

  Adobe has assigned part of the unicode public use area to hold some standard
  glyph variants like small caps, subscripts, old-style numbers, etc.
* Greek small caps

  I've added some greek small cap assignments
* TeX Names

  The TeX typesetting system has its own set of names
* AMS Names

  The American Mathematical Society has its set of names (see the American
  Mathematical Society's
  `specification <http://www.ams.org/STIX/bnb/stix-tbl.asc-2003-10-10>`__)

You may define your own namelist file. It should have the extension ".nam". And
it should contain a series of lines that look like:

::

   0x0020 space
   0x0021 exclam
   0x0022 dblquote

In many cases you will just want to make a few modifications to an already
existing namelist. You can write:

::

   Based: Adobe Glyph List
   0x0021 exclamation

Which means the namelist is the same as the Adobe Glyph List except that in your
system U+0021 will be called "exclamation" rather than "exclam".

Any glyphs you do not explicitly name will be named "uniXXXX" or "uXXXXX" where
XXXX is the unicode value in hex. The prefix "uni" will be used for glyphs in
the BMP, the prefix "u" for glyphs outside.

I said that glyph names should contain only alphanumeric characters (and some
others). But really this is only true of glyph names that get output into a
font. If you design your own namelist you may include whatever glyph names you
like, using (almost) the full range of unicode -- provided you rename all your
glyphs to Adobe's names before you generate a font. If you use non-ASCII
characters you should encode the file in utf-8.

FontForge normally will restrict glyph names to be within the ASCII range, but
if you go to
:menuselection:`File --> Preferences --> Generic --> UnicodeGlyphNames` and set
it on you can use unicode.

I've created a `french namelist <https://fontforge.org/downloads/ListeDesGlyphs.nam>`__
which uses accented letters. This will work fine within fontforge, and most rasterizers
will parse fonts generated using such names -- but they are non-standard and may
cause problems. Best to do a rename just before generating your font.

So if you find it easier to work with names other than those Adobe has
established you may create your own namelist file. Then use
:ref:`Encoding->Load NameList... <encodingmenu.LoadNL>` to load that into
FontForge (you only need do this once, FontForge should remember it thereafter).
You may use :ref:`File->Preferences->Font Info <prefs.NewFontNameList>` to
decree that all your new fonts will use this namelist. You can change a font's
namelist with either:

* :ref:`Element->Font Info->General->NameList <fontinfo.PS-General>`

  Which will change the way new glyphs are assigned names
* :ref:`Encoding->Rename Glyphs... <encodingmenu.RenameGlyphs>`

  Which will rename existing glyphs as well as changing the way new glyphs are
  named.

You may also want to force a rename of all glyphs when you Open a font, and the
open dialog now lets you do this. Similarly when generating a font you will
probably want to force that font to be use the standard names of the Adobe Glyph
List.
