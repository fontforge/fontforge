The CID Menu
============

The CID Menu provides a few commands for manipulating CID keyed fonts. If the
current font is a CID keyed font the menu also includes a list of all subfonts
that make up this one. This menu is only available in the font view.

.. warning:: 

   FontForge's support for CID-keyed fonts is rudimentary; especially attempting
   to use Compact encodings and the Metrics View in CID-keyed fonts is known to
   cause issues and crashes. At one time, the memory usage of CID-keyed fonts
   was seen as very worrisome (≈250MB), however as even most modern smartphones
   can support such usage it is no longer a concern for the majority of users.
   Therefore, we recommend that if you will make serious edits to a CID-keyed
   font, you :ref:`flatten <cidmenu.CID>` it first to prevent crashes and memory
   corruption. Bug fixes are always welcome-to start, see
   `issue №3946 <https://github.com/fontforge/fontforge/issues/3946#issuecomment-534005675>`__.


.. _cidmenu.CID:

Er... What is a CID keyed Font?
-------------------------------

A CID keyed font is a postscript (or opentype) font designed to hold Chinese,
Japanese and Korean characters efficiently. More accurately a CID font is a
collection of several sub-fonts each with certain common features (one might
hold all the latin letters, another all the kana, a third all the kanji). This
allows font-wide hinting to be crafted for subsets of glyphs to which have
something in common.

CID keyed fonts do not have an encoding built into the font, and the glyphs do
not have names. Instead the font is associated with a glyph set and on each
glyph set there are several character mappings defined. These mappings are
similar to encodings but allow for a wider range of behaviors.

A CID is a glyph index and is used to look up glyph descriptions instead of
glyph names in other types of fonts. Using a glyph set FontForge will often be
able to map a CID to a unicode character name (but not always), so FontForge
will give glyphs names when it can.

For more information see the
:ref:`section on CID keyed fonts on the font view page <fontview.CID>`.

.. _cidmenu.Convert:

.. object:: Convert to CID

   If the current font is not a CID font then this command will convert it into
   one containing one subfont (with the glyphs in this font). You will be
   prompted for a glyph set.

.. _cidmenu.ConvertCMap:

.. object:: Convert By CMap

   If the current font is not a CID font then this command will convert it into
   one containing a single subfont. You will be prompted for an Adobe CMap file.

.. _cidmenu.Flatten:

.. object:: Flatten

   If the current font is a CID font then this command will convert it into a
   normal (flat) font by taking all the glyphs from all the sub-fonts and
   merging them into one normal font. The new font should be in the same order
   as the CID font (ie. ordered by CID). After this operation you may re-encode
   it into whatever encoding is appropriate.

.. _cidmenu.FlattenCMap:

.. object:: Flatten By CMap

   If the current font is a CID font then this command will convert it into a
   normal font. It prompts you for an Adobe CMap file and uses that to define an
   encoding for the resultant font.

.. _cidmenu.Insert:

.. object:: Insert Font

   Will allow you to browse for a normal font which will be added as another sub
   font to the current CID font.

.. _cidmenu.Blank:

.. object:: Insert Blank

   Inserts a blank sub-font into the current CID font.

.. _cidmenu.Remove:

.. object:: Remove Font

   Removes the current font from the CID font. Anything in it will be lost. (If
   you want to save it first then use Generate Font and save it as a pfb file
   (or any other simple format).

.. _cidmenu.ChangeSup:

.. object:: Change Supplement...

   Displays the Registry/Ordering information of the font and allows you to
   change the Supplement level.

.. _cidmenu.FontInfo:

.. object:: CID Font Info

   This allows you to provide information on the entire collection of subfonts
   rather than just the current subfont. It provides access to the standard
   :doc:`font info dialog </ui/dialogs/fontinfo>`.

.. object:: <sub font name>

   Clicking on a different sub font name in the menu will cause that sub-font to
   be displayed instead of the current one.
