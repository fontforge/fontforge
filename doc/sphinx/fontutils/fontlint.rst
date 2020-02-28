fontlint
========

A program for checking the validity of fonts ::

   fontlint fontfile1 ...

*fontlint* checks a font for certain standard problems

It will notice:

* Any glyphs with intersecting (or self-intersecting) contours.
* Any contours drawn in the wrong orientation
* Any glyphs with a flipped reference
* Missing points at extrema.

For sfd fonts it will also notice:

* Any open contours (more an issue in sfd files than in released fonts)
* Adjacent points too far apart in a glyph
* Unknown glyph referenced in GSUB/GPOS/MATH

For PostScript fonts it will also notice:

* More points in a glyph than allowed by PostScript.
* Too many hints in a glyph.
* Invalid glyph name.
* Missing BlueValues entry in PostScript Private dictionary
* Odd number of elements in either the BlueValues or OtherBlues entries in the
  PostScript Private dictionary.
* Disordered elements in either the BlueValues or OtherBlues entries in the
  PostScript Private dictionary.
* Too many elements in either the BlueValues or OtherBlues entries in the
  PostScript Private dictionary.
* Elements too close in either the BlueValues or OtherBlues entries in the
  PostScript Private dictionary (must be at least 2*BlueFuzz+1 apart).
* Non-integral elements in either the BlueValues or OtherBlues entries in the
  PostScript Private dictionary.
* Alignment zone height in either the BlueValues or OtherBlues is too big for
  the BlueScale in the PostScript Private dictionary.
* Odd number of elements in either the FamilyBlues or FamilyOtherBlues entries
  in the PostScript Private dictionary.
* Disordered elements in either the FamilyBlues or FamilyOtherBlues entries in
  the PostScript Private dictionary.
* Too many elements in either the FamilyBlues or FamilyOtherBlues entries in the
  PostScript Private dictionary.
* Elements too close in either the FamilyBlues or FamilyOtherBlues entries in
  the PostScript Private dictionary (must be at least 2*BlueFuzz+1 apart).
* Non-integral elements in either the FamilyBlues or FamilyOtherBlues entries in
  the PostScript Private dictionary.
* Alignment zone height in either the FamilyBlues or FamilyOtherBlues is too big
  for the BlueScale in the PostScript Private dictionary.
* Bad BlueFuzz entry in PostScript Private dictionary.
* Bad BlueScale entry in PostScript Private dictionary.
* Bad StdHW entry in PostScript Private dictionary.
* Bad StdVW entry in PostScript Private dictionary.
* Bad StemSnapH entry in PostScript Private dictionary.
* Bad StemSnapV entry in PostScript Private dictionary.
* StemSnapH does not contain StdHW value in PostScript Private dictionary.
* StemSnapV does not contain StdVW value in PostScript Private dictionary.
* Bad BlueShift entry in PostScript Private dictionary.
* Bad 'CFF ' table.

For TrueType fonts it will also notice:

* More points in a glyph than specified in 'maxp'
* More paths in a glyph than specified in 'maxp'
* More points in a composite glyph than specified in 'maxp'
* More paths in a composite glyph than specified in 'maxp'
* Instructions longer than allowed in 'maxp'
* More references in a glyph than specified in 'maxp'
* References nested more deeply than specified in 'maxp'
* Bad 'glyf' or 'loca' table

In any sfnt (True or OpenType font):

* Bad sfnt table header, bad file/table checksums, overlapping tables, standard
  tables with incorrect lengths, etc.
* Bad PostScript fontname entry in the 'name' table
* Bad 'hhea', 'hmtx', 'vhea' or 'vmtx' table
* Bad 'cmap' table
* Bad 'EBDT', 'bdat', 'EBLC' or 'bloc' (embedded bitmap) table
* Bad Apple GX advanced typography table
* Bad OpenType advanced typography table
* Bad version number in OS/2 table (must be >0, and must be >1 for OT-CFF fonts)


See Also
--------

:doc:`fontforge </index>`