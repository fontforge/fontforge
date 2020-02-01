.. _sfdchangelog.change-log:

Changes to the sfd format
=========================

FontForge's :doc:`sfd (spline font database) format <sfdformat>` changes over
time as fontforge supports more things. I have only recently started keeping
track of these changes, so older changes are not listed here.

* 14-Sep-2010

  * Added gasp table version to GaspTable keyword.
* 03-Feb-2010

  * Added woff keywords (woffMajor, woffMinor (for version #) and woffMetadata for
    unparsed xml metadata).
  * Added sfntRevision keyword
* 22-Oct-2009

  * Add WidthSeparation keyword
  * Add extension to subtable specification to handle storing default separation,min
    kerning and touching values of the subtable in the sfd
  * Hmm. Document how vertical kerning subtables are indicated
  * Hmm. Document how default prefixes for single substitution subtables are stored.
* 12-May-2009

  * Add MarkAttachmentSet entries
  * Add names for features 'ss01'-'ss20'
* 15-Mar-2009

  * Add a BDFRefChar keyword to support bitmap references (Alexey Kryukov)
* 5-Mar-2009

  * Add support for the JSTF table in sfd.
* 12-Nov-2008

  * Add a field to the font: ExtremaBound. Any spline where the distance between its
    end points is less than or equal to this will not be checked for extrema.
* 22-Aug-2008

  * Add a flag to the Layer keyword (in the font) to say whether the layer is a
    background layer or not. As the flag is at the end of the line, previous
    versions of fontforge will just ignore it
* 10-July-2008

  * Extend the image output format to support red/green/blue/alpha images. (not much
    of an extension -- there's just more data)
* 26-May-2008

  * Adobe says UniqueID and XUID are deprecated. Add flags (UseXUID/UseUniqueID) to
    indicate we still want to include them in fonts in spite of that.
* 30-Mar-2008

  * Bump the sfd version number to 3. This is a bit late, but better late than
    never. It should have happened with the Layers release on 2-Mar.
  * Support for BASE table, new keywords

    * BaseHoriz
    * BaseVert
    * BaseScript

    This also involved removing the old vertical origin keyword which is (I hope) a
    subset of the BASEline information.
  * Support for Gradient and Pattern Fills in Type3 fonts, new keywords

    * FillGradient
    * StrokeGradient
    * FillPattern
    * StrokePattern
  * Added support for clipping paths in Type3 fonts, new keyword

    * PathFlags
* 2-Mar-2008

  * **Layers**

    .. warning:: 

       This turned out to be a change which was backwards incompatible. I did not
       realize that at the time

    All fonts may now have multiple layers (before only Type3 fonts could). The
    Type3 format has not changed so the discussion that follows concentrates on
    normal fonts and should be understood not to refer to type3 fonts.

    All glyphs in a given (non-type3) font have the same number of layers so there
    is a LayerCount keyword in the font which provides that (there is also a
    LayerCount keyword now in each glyph. This provides redundant information, it
    should be the same as the font's layer count (for non type3 fonts).

    Before this the foreground and background and Guide layers all had the same
    spline type (quadratic or cubic), now that can vary from layer to layer. There
    is a font level keyword "Layer" which provides the spline order and name of each
    layer. The old "Order2" keyword has been removed.

    Previously only the foreground layer of a glyph could contain references, now
    any layer can. Before the "Refer" keyword could appear with no layer name
    preceding it (as this unambiguously meant that the references would go into the
    foreground layer), now there must be a layer name preceding a "Refer" keyword.

    Each layer output is preceded by one of "Fore", "Back" or "Layer: d" (where "d"
    is the layer number).

    Before the SplineSet keyword was not needed (and was usually omitted) after a
    "Fore" or "Back" layer name. It is now required, and this seems to render new
    sfd files incompatible with old.

    (If there is no layer tag before a Refer keyword then the parser will apply it
    to the foreground layer)

    Previously the validation status applied only to the foreground layer. Now every
    layer can have its own status, so again the "Validated" keyword must now be
    tagged with a layer.

    There is a font level keyword "DisplayFont" which indicates what layer should be
    displayed in the font view by default.

    There is a font level keyword "GridOrder2" which may appear before the "Grid"
    layer name in the font to set the spline order of the guideline semi-layer.
* 3-Feb-2008

  * The point flags bitmap in a SplineSet has been extended by

    0x200 -- Any extrema on the preceding spline are marked as acceptable to the
    validator
  * Some sfnt tables are now output in text instead of binary.

    * cvt,maxp -- output as decimal numbers (with possibly with comments for cvt)
    * prep,fpgm -- output as ASCII instructions
  * Get diagonal hints working again
  * Add a FontLog field
* 9-Nov-2007

  * Add support for Raph Levien's spiro curves.
  * Add the ability to name contours
* ...
