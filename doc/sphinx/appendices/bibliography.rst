A meandering bibliography of font related things
================================================

.. _bibliography.Formats:

Font File Formats
-----------------

* `PostScript Type1 <http://partners.adobe.com/public/developer/en/font/T1_SPEC.PDF>`__

  * `Supplement <http://partners.adobe.com/public/developer/en/font/5015.Type1_Supp.pdf>`__
    (discussion of multiple master fonts & counter hints)
  * `Format <http://partners.adobe.com/public/developer/en/font/T1Format.pdf>`__
  * `PostScript unicode character names <http://partners.adobe.com/public/developer/opentype/index_glyph.html>`__
  * `PostScript Language Reference Manual <http://www.adobe.com/products/postscript/pdfs/PLRM.pdf>`__
* PostScript Multiple Master

  * `Type1 MM format specification (in the Type1 Supplement) <http://partners.adobe.com/public/developer/en/font/5015.Type1_Supp.pdf>`__
  * `Design considerations <http://partners.adobe.com/public/developer/en/font/5091.Design_MM_Fonts.pdf>`__
  * `Naming requirements <http://partners.adobe.com/public/developer/en/font/5088.FontNames.pdf>`__
  * Type2 (In March of 2000, Adobe removed multiple master support from Type2 and
    CFF files)

    * `Type2 MM format specification <http://ftp.ktug.or.kr/obsolete/info/adobe/devtechnotes/pdffiles/5177.Type2.pdf>`__
      (In **OBSOLETE** type2 spec)
    * `CFF MM format specification <http://ftp.ktug.or.kr/obsolete/info/adobe/devtechnotes/pdffiles/5176.CFF.pdf>`__
      (In **OBSOLETE** CFF spec)
* `CID keyed fonts <http://partners.adobe.com/public/developer/en/font/5014.CMap_CIDFont_Spec.pdf>`__
* `PostSript Type2 <http://partners.adobe.com/public/developer/en/font/5177.Type2.pdf>`__

  * `Compact Font Format Specification <http://partners.adobe.com/public/developer/en/font/5176.CFF.pdf>`__
    (CFF)
  * For more information see under :ref:`OpenType fonts <bibliography.OpenType>`
* PostScript Type3

  * `PostScript Language Reference Manual 3.0 <http://www.adobe.com/products/postscript/pdfs/PLRM.pdf>`__
    (see section 5.7)
* PostScript Type14 (Chameleon)

  * The PLRM (5.8.1) documents that this font format is undocumented.
* `PostScript Type42 <http://partners.adobe.com/public/developer/en/font/5012.Type42_Spec.pdf>`__
* `Adobe Feature File (fea) <http://www.adobe.com/devnet/opentype/afdko/topic_feature_file_syntax.html>`__

  * (:doc:`FontForge's implementation </techref/featurefile>` of this format is a superset of
    what Adobe accepts, and a superset of what Adobe documents. Neither can
    completely describe opentype. Adobe claims they will update the feat spec in
    late 2007).
* `AFM <http://partners.adobe.com/public/developer/en/font/5004.AFM_Spec.pdf>`__
* PFM

  * I can't find microsoft's docs for pfm files any more, I think the format may be
    obsolete having been replaced by ntf.
  * `Adobe's notes on PFM files for two byte fonts <http://partners.adobe.com/public/developer/en/font/5178.PFM.pdf>`__
  * `Third Party description <http://homepages.muenchen.org/bm134751/pfm_fmt_en.html>`__
* `NTF <http://msdn.microsoft.com/library/default.asp?url=/library/en-us/graphics/hh/graphics/pscript_7twn.asp>`__

  * This format is supposed to replace the pfm files above in windows >2000. I can't
    find any docs on it.
* `BDF <http://partners.adobe.com/public/developer/en/font/5005.BDF_Spec.pdf>`__

  * `X11 Long Font Descriptor <http://ftp.xfree86.org/pub/XFree86/4.5.0/doc/xlfd.txt>`__
    spec defines standard X BDF Properties
  * `ABF <http://partners.adobe.com/public/developer/en/font/5006.ABF_Spec.pdf>`__
    -- Binary format
  * :doc:`Extensions to BDF for greymap support </techref/BDFGrey>`
* True Type Standard

  (Sadly different sources have slightly different definitions of less important
  parts of the standard, be warned)

  * `Apple <http://developer.apple.com/fonts/TTRefMan/>`__ (I find Apple's prose
    difficult, and sometimes misleading. I suggest using a different source when
    possible)
  * `Microsoft <http://www.microsoft.com/typography/tt/tt.htm>`__
  * `random useful site <http://www.truetype.demon.co.uk/ttspec.htm>`__
  * `TTC <http://partners.adobe.com/asn/tech/type/opentype/otff.jsp>`__ -- True Type
    Font Collection
* `Apple Advanced Typography <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6.html>`__
  extensions to TrueType
* Apple distortable font (variation tables) -- vaguely equivalent to Multiple
  Master fonts for TrueType

  * `fvar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6fvar.html>`__ (font
    variations)
  * `gvar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6gvar.html>`__ (glyph
    variations)
  * `cvar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6cvar.html>`__ (cvt
    variations)
  * `avar <http://developer.apple.com/fonts/TTRefMan/RM06/Chap6avar.html>`__ (axis
    variations)
* .. _bibliography.OpenType:

  `OpenType <http://partners.adobe.com/public/developer/opentype/index_spec.html>`__
  (postscript embedded in a truetype wrapper, or advanced typography tables in a
  truetype wrapper)

  * PostScript
    `Type2 <http://partners.adobe.com/public/developer/en/font/5177.Type2.pdf>`__
  * `CFF <http://partners.adobe.com/public/developer/en/font/5176.CFF.pdf>`__
  * `Adobe's version of file format <http://partners.adobe.com/public/developer/opentype/index_spec.html>`__

    * `SING Gaiji extention <http://partners.adobe.com/public/developer/opentype/gdk/topic.html>`__
      (more information is available in the documentation subdirectory of the Glyphlet
      GDK)
  * `Microsoft's version <http://www.microsoft.com/typography/otspec/default.htm>`__
  * Possible source of script codes for scripts not specified by MS/Adobe:
    `ISO 15924 <http://www.evertype.com/standards/iso15924/document/dis15924.pdf>`__
  * `Microsoft's full list of locale/language IDs <http://www.microsoft.com/globaldev/reference/lcd-all.mspx>`__
    (not all are supported, some may never be)
* Open Font Format Specification (ISO/IEC 14496-22:2007)

  (based on OpenType 1.4 but an international standard)
* `Apple's sfnt wrapper around a PS type1 font <ftp://ftp.apple.com/developer/Development_Kits/QuickDraw_GX/Documents.sit.hqx>`__
* :doc:`Various bitmap only sfnt formats </techref/bitmaponlysfnt>`
* `WOFF <http://people.mozilla.com/~jkew/woff/woff-2009-09-16.html>`__ -- Web Open
  Font Format, mozilla's compressed sfnt format
* `PostScript Type42 <http://partners.adobe.com/public/developer/en/font/5012.Type42_Spec.pdf>`__
  (the opposite of opentype, it's truetype embedded in postscript)
* SVG 1.1 `fonts <http://www.w3c.org/TR/SVG11/fonts.html>`__

  * `SVG 1.2 font hinting proposal <http://www.w3c.org/TR/SVG12/>`__
* :doc:`Macintosh font formats </techref/macformats>`
* Windows raster font formats

  * `FNT -- Windows version 2 <http://www.technoir.nu/hplx/hplx-l/9708/msg00404.html>`__
  * `FNT -- Windows version 3 <http://support.microsoft.com/default.aspx?scid=KB;en-us;q65123>`__
  * `Some info on FON file format <http://www.csn.ul.ie/~caolan/publink/winresdump/winresdump/doc/resfmt.txt>`__
* X11 pcf format

  * Sadly there is no real standard for this.
    `There's the source code used by X11 <http://ftp.x.org/pub/R6.4/xc/lib/font/bitmap/>`__.
  * :doc:`So I wrote my own description... </techref/pcf-format>`
* `PC Screen Font (psf/psfu/psf2) <http://www.win.tue.nl/~aeb/linux/kbd/font-formats-1.html>`__
* TeX font formats

  * `pk packed bitmap format <http://www.ctan.org/tex-archive/systems/knuth/local/mfware/pktype.web>`__
  * `gf generic font (bitmap) format <http://www.ctan.org/tex-archive/systems/knuth/mfware/gftype.web>`__
  * `tfm metrics format <http://www.ctan.org/tex-archive/systems/knuth/texware/tftopl.web>`__
  * To make these viewable you probably want to do something like:

    $ weave pktype.web

    $ pdftex pktype.tex
* `SIL Graphite Fonts <http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&cat_id=RenderingGraphite>`__
  (smart font extension to TrueType. Additional tables containing rules for
  composing, reordering, spacing, etc. glyphs)
* Palm pilot fonts (pdb files)

  * `font record format <http://www.palmos.com/dev/support/docs/palmos/PalmOSReference/Font.html>`__
  * `pdb file format <http://www.palmos.com/dev/support/docs/fileformats/Intro.html#970318>`__
* `OpenDoc <http://www.bitstream.com/categories/developer/truedoc/pfrspec.html>`__.
  Sadly Proprietary so I shan't support it.
* `Acorn RISC OS font format <http://www.pinknoise.demon.co.uk/Docs/Arc/Fonts/Formats.html>`__
  (these fonts are often zipped up with a non-standard zip).
* Ikarus IK format is documented in Peter Karow's book * Digital Formats for
  Typefaces,* Appendices G&I. (copies may still be available from
  `URW++ <http://www.urwpp.de/english/home.htm>`__)

  Interestingly the exact format of a curve is up to the interpretation program.
* :doc:`sfd </techref/sfdformat>` files (FontForge's internal spline font database format)
* :doc:`cidmap </techref/cidmapformat>` files (Fontforge's format for mapping cids to
  unicode)
* XML formats

  * TTX -- TrueType XML
  * `UFO <http://unifiedfontobject.org/>`__ &
    `GLIF <http://unifiedfontobject.org/storageformats/glif.html>`__ -- Unified font
    objects & Glyph Interchange Format

Other font links

* `Adobe's downloadable font spec <http://partners.adobe.com/public/developer/en/font/5040.Download_Fonts.pdf>`__
* `Adobe's technical notes <http://partners.adobe.com/asn/tech/type/ftechnotes.jsp>`__
* `Adobe's Font Policies document <http://partners.adobe.com/asn/acrobat/sdk/public/docs/FontPolicies.pdf>`__
* `PostScript reference manual <http://www.adobe.com/products/postscript/pdfs/PLRM.pdf>`__

  * (old
    `reference manual <http://partners.adobe.com/asn/developer/pdfs/tn/psrefman.pdf>`__)
* `Microsoft's downloadable fonts <http://www.microsoft.com/typography/fontpack/default.htm>`__
* `Downloadable PS CID CJK fonts <ftp://ftp.ora.com/pub/examples/nutshell/ujip/adobe/samples/>`__
  (this site also has cmap
  files)`others <ftp://ftp.ora.com/pub/examples/nutshell/cjkv/adobe/samples/>`__
* `Downloadable OTF CID CJK fonts <http://www.adobe.com/products/acrobat/acrrasianfontpack.html>`__
  (this site also has cmap files)
* `Most recent cid2code tables that I'm aware of <ftp://ftp.oreilly.com/pub/examples/nutshell/cjkv/adobe>`__
* PANOSE

  * `PANOSE Classification Metrics Guide <http://panose.com>`__ by Hewlett-Packard
    Corporation, 1991 - 1997
  * `PANOSE structure (Windows) <https://msdn.microsoft.com/en-us/library/windows/desktop/dd162774(v=vs.85).aspx>`__
  * `PANOSE: An Ideal Typeface Matching System for the Web <https://www.w3.org/Printing/stevahn.html>`__
    by Robert Stevahn, 1996
  * `PANOSE 2.0 White Paper <https://www.w3.org/Fonts/Panose/pan2.html>`__ by
    Hewlett-Packard Corporation, 1993
  * `PANOSE <https://en.wikipedia.org/wiki/PANOSE>`__ on Wikipedia
  * `Classifying Arabic Fonts Based on Design Characteristics: PANOSE-APANOSE <http://spectrum.library.concordia.ca/981753/>`__
    by Jehan Janbi, 2016


Related software
----------------

* `Gimp <http://www.gimp.org/>`_
* `Gimp users group <http://gug.sunsite.dk/>`_



.. _bibliography.Unicode:

Unicode
-------

* `Unicode consortium <http://www.unicode.org/>`__

  * `Apple's corporate use extensions <http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/CORPCHAR.TXT>`__
    (0xF850-0xF8FE)
  * `Adobe's corporate use extensions <http://partners.adobe.com/asn/tech/type/type/corporateuse.txt>`__
    (0xF634-0F7FF) (also includes some of Apple's codes above)
  * :doc:`FontForge's corporate use extensions </techref/corpchar>` (0xF500-0xF580)
  * `A registry of code points in the private area <http://www.evertype.com/standards/csur/>`__
    (does not include any of Adobe's or Apple's codepoints)
  * `American Mathematical Society's corporate use extensions <http://www.ams.org/STIX/bnb/stix-tbl.asc-2003-10-10>`__
    (0xE000-0xF7D7)
  * MicroSoft uses 0xF000-0xF0FF in their "Symbol" encoding (3,0) when they want to
    an uninterpretted encoding vector (ie. a mapping from byte to glyph with no
    meaning attached to the mapping)
* `Unicode en français <http://hapax.qc.ca/>`__
* `Pictures of the characters <http://www.unicode.org/charts/>`__
* `Unicode script assignments <http://www.unicode.org/Public/UNIDATA/Scripts.txt>`__

  * `ISO 15924 script list <http://www.unicode.org/iso15924-en.html>`__
* `Unicode Bloopers <http://www.babelstone.co.uk/Unicode/Bloopers.html>`__
* `PostScript Unicode names <http://partners.adobe.com/public/developer/opentype/index_glyph.html>`__

  * `Glyph names for new fonts <http://partners.adobe.com/public/developer/en/opentype/aglfn13.txt>`__
    (these are the names FontForge automatically assigns to glyphs)
  * `Adobe Glyph Names <http://partners.adobe.com/public/developer/en/opentype/glyphlist.txt>`__
    provides further synonyms
  * `Glyph name limitations <http://partners.adobe.com/public/developer/opentype/index_glyph2.html>`__
* Linux issues

  * `FAQ <http://www.cl.cam.ac.uk/~mgk25/unicode.html>`__
  * `HOWTO <ftp://ftp.ilog.fr/pub/Users/haible/utf8/Unicode-HOWTO.html>`__
  * `Linux Unicode man page <http://bobo.fuw.edu.pl/cgi-bin/man2html/usr/share/man/man7/unicode.7.gz>`__


.. _bibliography.Encodings:

Other Encodings
^^^^^^^^^^^^^^^

* `Microsoft's Codepages <http://www.microsoft.com/globaldev/reference/wincp.asp>`__,
  and at the
  `unicode site <http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WINDOWS/>`__
* `Mac Encodings <http://www.unicode.org/Public/MAPPINGS/VENDORS/APPLE/>`__
* `MacRoman <http://devworld.apple.com/techpubs/mac/Text/Text-516.html>`__
* `IPA <http://www2.arts.gla.ac.uk/IPA/fullchart.html>`__
* `GB 18030 <http://www-106.ibm.com/developerworks/unicode/library/u-china.html?dwzone=unicode>`__
* `TeX latin encodings <http://www.tug.org/fontname/html/Encodings.html>`__
  (possibly also on your local machine in ``/usr/share/texmf/dvips/base``)
* `TeX cyrillic encodings <http://www.ctan.org/tex-archive/macros/latex/contrib/supported/t2/enc-maps/encfiles/>`__

--------------------------------------------------------------------------------


.. _bibliography.Books:

Books
-----


.. _bibliography.FontForge:

FontForge
^^^^^^^^^

* .. image:: http://images-eu.amazon.com/images/P/284177273X.08.MZZZZZZZ.jpg
     :align: left
     :alt: Fontes et Codages

  `Haralambous, Yannis, 2004, Fontes & Codages <http://www.amazon.fr/exec/obidos/ASIN/284177273X/qid%3D1096481415/402-5423443-8577732>`__

* .. image:: http://images.amazon.com/images/P/0596102429.01._AA240_SCLZZZZZZZ_V40077239_.jpg
     :align: left
     :alt: Fontes et Codages

  `Haralambous, Yannis (translated: P Scott Horne), 2006, Fonts & Encodings <http://www.amazon.com/Fonts-Encodings-Yannis-Haralambous/dp/0596102429/sr=1-1/qid=1158862933/ref=sr_1_1/103-9032945-8593416?ie=UTF8&s=books>`__


.. _bibliography.Typography:

Typography
^^^^^^^^^^


.. _bibliography.editor:

Font editor concepts
^^^^^^^^^^^^^^^^^^^^

Karow, Peter, 1994, *Font Technology, Description and Tools*

Karow, Peter, 1987, *Digital Formats for Typefaces*


.. _bibliography.TeX:

TeX
^^^

Hoenig, Alan *TeX Unbound: LaTeX and TeX Strategies for Fonts, Graphics & More*

Knuth, Donald, 1979, *TeX and METAFONT, New Directions in Typesetting*


Interview
---------

I was interviewed by the Open Source Publishing people at
`LGM2 <http://www.libregraphicsmeeting.org/>`__. There's an
`mp3 file of the interview available on their site. <http://ospublish.constantvzw.org/?p=221>`__