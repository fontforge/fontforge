TrueType and OpenType tables supported by FontForge
===================================================

`Apple <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6.html>`__,
`MS <http://www.microsoft.com/typography/tt/tt.htm>`__ and
`Adobe <http://partners.adobe.com/public/developer/opentype/index_spec.html>`__
all have their descriptions of TrueType and OpenType. MS and Adobe's versions
are essentially the same, while Apple's differ significantly from either.

.. list-table:: 
   :header-rows: 1
   :widths: 5 5 80 5 5

   * - name
     - long name
     - FontForge's use
     - Apple docs
     - Adobe docs
   * - avar
     - axis variation
     - Used to specify piecewise linear sections of axes for distortable fonts
     - `avar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6avar.html>`__
     - *
   * - BASE
     - baseline data
     - Provides information on baseline positioning and per-script line heights in
       OpenType.
     - *
     - `BASE <http://partners.adobe.com/public/developer/opentype/index_base.html>`__
   * - bsln
     - baseline data
     - Provides information on baseline positioning for AAT.
     - `bsln <http://developer.apple.com/textfonts/TTRefMan/RM06/Chap6bsln.html>`__
     - *
   * - BDF
     - X11 BDF header
     - This table is not part of the official TrueType or OpenType specs. It
       contains the header information from old bdf fonts. I describe its format
       :doc:`here <non-standard>`.
     - *
     - *
   * - bdat
     - bitmap data
     - Provides the actual bitmap data for all bitmap glyphs in Apple fonts. 'EBDT'
       is used by MS/Adobe instead (and has the same format)
     - `bdat <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6bdat.html>`__
     - *
   * - bhed
     - bitmap font header
     - Replaces the 'head' table in Apple's bitmap only fonts. (similar to 'head')
     - `bhed <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6bhed.html>`__
     - *
   * - bloc
     - bitmap location data
     - Provides pointers to the appropriate bitmap data for each bitmap glyph in
       Apple fonts. 'EBLC' is used by MS/Adobe instead (and has the same format)
     - `bloc <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6bloc.html>`__
     - *
   * - CFF
     - `Compact Font Format <http://partners.adobe.com/asn/developer/pdfs/tn/5176.CFF.pdf>`__
     - Provides all the outline data (and sometimes glyph names as well) for the
       PostScript
       `Type2 <http://partners.adobe.com/asn/developer/pdfs/tn/5177.Type2.pdf>`__
       font that is the heart of an OpenType font. Vaguely equivalent to 'glyf' ,
       'loca' and 'post' combined.
     - *
     - `CFF <http://partners.adobe.com/public/developer/opentype/index_cff.html>`__
   * - cmap
     - character code mapping
     - Provides at least one encoding from a stream of bytes to glyphs in the font.
       FontForge provides at least three encodings in every font (one for Mac Roman,
       one for Mac Unicode, one for MS Unicode). For some fonts it will additionally
       provide a CJK encoding (Big5, SJIS, etc) or an extended unicode encoding
       which will cover code points outside of the BMP.

       For fonts with special 1-byte encodings (such as symbol) it will provide a
       "symbol" encoding which maps a known page in the corporate use area to the
       glyphs.
     - `cmap <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6cmap.html>`__
     - `cmap <http://partners.adobe.com/public/developer/opentype/index_cmap.html>`__
   * - cvar
     - variations on cvt table
     - Used to specify hinting differences in distortable fonts
     - `cvar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6cvar.html>`__
     - *
   * - cvt
     - control value table
     - FontForge uses this when it attempts to hint truetype fonts (FontForge will
       read it from a font and save it into another).
     - `cvt <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6cvt.html>`__
     - `cvt <http://partners.adobe.com/public/developer/opentype/index_cvt.html>`__
   * - EBDT
     - embedded bitmap data
     - Provides the actual bitmap data for all bitmap glyphs in MS/Adobe fonts.
       'bdat' is used by Apple instead (and has the same format)
     - *
     - `EBDT <http://partners.adobe.com/public/developer/opentype/index_ebdt.html>`__
   * - EBLC
     - embedded bitmap location
     - Provides pointers to the appropriate bitmap data for each bitmap glyph in
       MS/Adobe fonts. 'bloc' is used by Apple instead (and has the same format)
     - *
     - `EBLC <http://partners.adobe.com/public/developer/opentype/index_eblc.html>`__
   * - EBSC
     - embedded bitmap scaling
     - Provides information on how to scale bitmaps (on those rare occasions where
       this is desirable). FontForge uses it when making bitmap only fonts for
       windows.
     - `EBSC <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6EBSC.html>`__
     - `EBSC <http://partners.adobe.com/public/developer/opentype/index_ebsc.html>`__
   * - feat
     - layout feature table
     - Maps features specified in Apple's 'morx' (or 'mort') tables into names
       provided in the 'name' table. FontForge generates this when it generates a
       'morx' table.
     - `feat <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6feat.html>`__
     - *
   * - FFTM
     - FontForge timestamp
     - This table is unique to FontForge. It contains three timestamps: First
       FontForge's version date, then when the font was generated, and when the font
       was created. I describe its format :doc:`here <non-standard>`.
     - *
     - *
   * - fpgm
     - font program
     - FontForge never generates this, but it will preserve it and allows users to
       edit it.
     - `fpgm <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6fpgm.html>`__
     - `fpgm <http://partners.adobe.com/public/developer/opentype/index_fpgm.html>`__
   * - fvar
     - font variations
     - Provides top level information about distortable fonts. Specifies the types
       of distortions possible.
     - `fvar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6fvar.html>`__
     - *
   * - gasp
     - grid-fitting and scan conversion
     - When FontForge does not attempt to hint a truetype font it will generate this
       table which tells the rasterizer not to to do grid-fitting (hinting) but to
       do anti-aliasing instead.
     - `gasp <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6gasp.html>`__
     - `gasp <http://partners.adobe.com/public/developer/opentype/index_gasp.html>`__
   * - GDEF
     - glyph definition
     - Divides glyphs into various classes that make using the GPOS/GSUB tables
       easier. Also provides internal caret positions for ligatures. It is
       approximately equivalent to Apple's 'prop' and 'lcar' tables combined.
     - *
     - `GDEF <http://partners.adobe.com/public/developer/opentype/index_table_formats5.html>`__
   * - glyf
     - glyph outline
     - Provides outline information for truetype glyphs. Vaguely equivalent to 'CFF
       '.
     - `glyf <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6glyf.html>`__
     - `glyf <http://partners.adobe.com/public/developer/opentype/index_glyf.html>`__
   * - GPOS
     - glyph positioning
     - Provides kerning information, mark-to-base, etc. for opentype fonts. See the
       :ref:`chapter on GPOS <gposgsub.GPOS>` in FontForge for more information.
       Vaguely equivalent to 'kern' and 'opbd' tables.
     - *
     - `GPOS <http://partners.adobe.com/public/developer/opentype/index_table_formats1.html>`__
   * - GSUB
     - glyph substitution
     - Provides ligature information, swash, etc. for opentype fonts. See the
       :ref:`chapter on GSUB <gposgsub.GSUB>` in FontForge for more information.
       Vaguely equivalent to 'morx'.
     - *
     - `GSUB <http://partners.adobe.com/public/developer/opentype/index_table_formats1.html>`__
   * - gvar
     - glyph variations
     - This table contains the meat of a distortable font. It specifies how each
       glyph can be distorted.
     - `gvar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6gvar.html>`__
     - *
   * - head
     - font header
     - Contains general font information, such as the size of the em-square, a time
       stamp, check sum information, etc.
     - `head <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6head.html>`__
     - `head <http://partners.adobe.com/public/developer/opentype/index_head.html>`__
   * - hhea
     - horizontal header
     - This table contains font-wide horizontal metric information.
     - `hhea <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6hhea.html>`__
     - `hhea <http://partners.adobe.com/public/developer/opentype/index_hhea.html>`__
   * - hmtx
     - horizontal metrics
     - This contains the per-glyph horizontal metrics.
     - `hmtx <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6hmtx.html>`__
     - `hmtx <http://partners.adobe.com/public/developer/opentype/index_hmtx.html>`__
   * - kern
     - kerning
     - Provides kerning information for Apple's truetype fonts (and for older MS
       truetype fonts). In OpenType fonts this information is contained in the GPOS
       table.
     - `kern <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6kern.html>`__
     - `kern <http://partners.adobe.com/public/developer/opentype/index_kern.html>`__
   * - lcar
     - ligature caret
     - This table provides the location of carets within ligatures in an Apple font.
       This information is contained in the GDEF table in opentype fonts.
     - `lcar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6lcar.html>`__
     - *
   * - loca
     - glyph location
     - This provides a pointer into the 'glyf' table for each glyph in the font. It
       is required for truetype and meaningless for opentype (where vaguely
       equivalent information is provided in the CFF table).
     - `loca <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6loca.html>`__
     - `loca <http://partners.adobe.com/public/developer/opentype/index_loca.html>`__
   * - MATH
     - mathematical typesetting
     - Provides general information needed for mathematical typesetting.

       This is a new table (August 2007) and there is currently no publicly
       available documentation for it.
     - *
     - *
   * - maxp
     - maximum profile
     - Provides general "maximum" information about the font. This contains: the
       maximum number of glyphs in a font, the maximum number of points in a glyph,
       the maximum number of references in a glyph, etc.
     - `maxp <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6maxp.html>`__
     - `maxp <http://partners.adobe.com/public/developer/opentype/index_maxp.html>`__
   * - mort
     - metamorphosis
     - FontForge will read this table but not generate it. This table has been
       replaced by the 'morx' table which will be generated instead. It is vaguely
       equivalent to the GSUB table.
     - `mort <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6mort.html>`__
     - *
   * - morx
     - extended metamorphosis
     - Provides ligature information, swash, etc. for apple's truetype fonts.
       FontForge can read and write this table. See the
       :ref:`chapter on morx <gposgsub.AAT>` in FontForge for more information.
       Vaguely equivalent to 'GSUB'.
     - `morx <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6morx.html>`__
     - *
   * - name
     - name
     - Provides certain standard strings relevant to the font (font name, family
       name, license, etc.) in various languages. In Apple fonts also provides the
       names of various features of the 'morx'/'mort' table.
     - `name <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6name.html>`__
     - `name <http://partners.adobe.com/public/developer/opentype/index_name.html>`__
   * - opbd
     - optical bounds
     - This table provides optical bound information for each glyph in the table in
       an Apple font. This information may also be provided in the GPOS table of an
       opentype font.
     - `opbd <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6opbd.html>`__
     - *
   * - PfEd
     - FontForge's personal table
     - This table is unique to FontForge. It can contain things like FontForge's
       comments, and colors.I describe its format :doc:`here <non-standard>`.
     - *
     - *
   * - post
     - glyph name and postscript compatibility
     - This table provides some additional postscript information (italic angle),
       but mostly it provides names for all glyphs.
     - `post <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6post.html>`__
     - `post <http://partners.adobe.com/public/developer/opentype/index_post.html>`__
   * - prep
     - cvt program
     - FontForge never generatest this table itself, but it will retain it when
       reading other fonts, and it allows users to edit it.
     - `prep <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6prep.html>`__
     - `prep <http://partners.adobe.com/public/developer/opentype/index_prep.html>`__
   * - prop
     - glyph properties
     - Provides the unicode properties of each glyph for an Apple font. This table
       bears some similarities to the GDEF table.
     - `prop <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6prop.html>`__
     - *
   * - TeX
     - TeX information
     - This table is unique to FontForge and is for use by TeX. I describe its
       format :doc:`here <non-standard>`.
     - *
     - *
   * - vhea
     - vertical header
     - FontForge generates this table for fonts with vertical metrics. This table
       contains font-wide vertical metric information.
     - `vhea <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6vhea.html>`__
     - `vhea <http://partners.adobe.com/public/developer/opentype/index_vhea.html>`__
   * - vmtx
     - vertical metrics
     - FontForge generates this table for fonts with vertical metrics. This contains
       the per-glyph vertical metrics.
     - `vmtx <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6vmtx.html>`__
     - `vmtx <http://partners.adobe.com/public/developer/opentype/index_vmtx.html>`__

:doc:`Advanced Typography tables in FontForge <gposgsub>`