Caveats about References
========================

Different font formats place different restrictions on the use of references.
This does NOT mean you should avoid references -- they are a useful way of
encapsulating information even if the font format cannot support them directly.
If FontForge finds a situation where it can't use a referred glyph as a reference
it will fix it up somehow, usually this means replacing it with the equivalent
outlines.

Let us examine the most common font formats:

.. object:: TrueType

   In TrueType any glyph may be referenced, and TrueType supports almost a full
   range of linear transformations that may be applied to a reference, however
   if a reference is scaled by 200% or more (or -200% or less) it cannot be
   represented in TrueType.

   TrueType also does not support mixing outlines and references.

   If you have a glyph containing a reference which cannot be used (or that
   mixes outlines and references) then ALL references will be converted to
   outlines during output.

   This means that any instructions in the glyph itself will be meaningless, and
   any instructions in referred glyphs will not be executed.

   NOTE: Just because general transformations are supported, it isn't always a
   good idea to use them. If you flip a reference, the rasterizer will probably
   have difficulties with it (its contours will run in the wrong direction). If
   you rotate a reference any instructions inside it will not work well.

.. object:: Type1

   Type1 (and Type2) fonts have the basic limitation that the only
   transformation that may be applied to a reference is translation no scaling,
   rotating or flipping are allowed. Type1 fonts have two different mechanisms
   for using references. The first is the simplest to describe.

   If you have a glyph which contains exactly two references to characters which
   are themselves part of the Adobe Standard Encoding, and one of those
   references has an identity transformation matrix applied to it (that is, it
   is not moved, scaled, rotated, flipped, etc.) and the width of the composite
   glyph is the same as the width of this referred glyph and the other is only
   translated (possibly by 0, but not scaled, rotated, flipped, etc.) THEN
   FontForge can generate two references (this is done with the 'seac'
   instruction). CID-keyed fonts do not support this.

   The implications of this are that this form of referencing is useless for
   non-latin scripts (except for glyphs shared by latin and another script,
   Greek and Cyrillic often share the glyph used for latin A).

   FontForge makes a slight extension, in that if you have a glyph which
   contains one single untranslated reference, then FontForge will add a dummy
   reference to the space glyph to make it fit the two reference requirement.

   The second format is more general in some ways but has more arcane
   restrictions imposed on it. PostScript fonts have the concept of
   "subroutines" which can be used to define the contours of several glyphs.
   FontForge's algorithm is quite complex and can depend on what other glyphs
   are referred to. But basically if a glyph contains no hint substitutions, nor
   flex hints it can be put into a subroutine. If it does contain these it can
   be put into a subroutine if it has not been translated. Even if one reference
   cannot be put into a subroutine, another may be (ie. TrueType references are
   an all or nothing affair, that is not true of PostScript subroutines). If a
   reference cannot be put in a subroutine, FF may still be able to put some of
   its components (assuming it is a glyph with references) in a subroutine.

   FontForge will recognize 'seac' as defining references, but it does not
   recognize subroutine calls. Use
   :ref:`Edit->Replace With Reference <editmenu.ReplaceRef>` after loading a
   Type1 font.

   FontForge can also break an outline into smaller segments and place those in
   subroutines these may be shared more easily among glyphs. They are no longer
   true references, but they will make the output font smaller.

.. object:: Type2 ("OpenType")

   As above references may only be translated (not scaled, rotated, etc.). Type2
   does not have anything equivalent to the 'seac' instruction (well, it sort of
   does, but the instruction is depreciated and FontForge will not generate it)
   but does support subroutines.

   Here a referenced glyph can be put into a subroutine if it contains no hint
   substitutions, or if it is not translated and none of the other components of
   the final composite contain any hints at all. Again this is not an all or
   nothing affair.

   And again FontForge can break glyphs into smaller segements which can be
   placed in subroutines and shared among glyphs. These are not true references
   but do make the font smaller.

   FontForge will not recognize any references when loading a Type2 (otf, cff,
   cef, gai) font. Use :ref:`Edit->Replace With Reference <editmenu.ReplaceRef>`
   after loading the font.

.. object:: Type3

   There are no restrictions on references.

.. object:: SVG

   Does not seem to have an easy way of handling references.
