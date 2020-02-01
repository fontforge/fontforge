Several formats for bitmap only sfnts
=====================================

:small:`(the file type which holds a truetype or opentype font)`

Unfortunately every system has its own way of storing bitmap only fonts into an
sfnt wrapper (or the system just doesn't support it)


.. _bitmaponlysfnt.Apple:

Apple
-----

Apple documents the existence of a bitmap only format, and gives some hints
about the requirements of it. Their documentation is far from complete and the
following is determined in part by that documentation, in part by examining the
(few) bitmap only fonts of theirs I have found, and in part by error messages
given by some of their tools.

* As is expected on Apple, the bitmap data reside in '`bloc`_' and '`bdat`_' tables.

  (These are identical in format to the '`EBLC`_' and '`EBDT`_' tables used in
  OpenType)
* The '`head <https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6head.html>`__'
  table is replaced by a '`bhed`_' table which is byte for byte identical
* There are no '`glyf`_', '`loca`_' nor '``CFF`` ' tables.
* There are no '``hhea``' nor '``hmtx``' tables (metrics data are provided in
  the bitmap strikes themselves)
* (Presumably there are no '``vhea``' nor '``vmtx``' tables either)
* '`maxp <https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6maxp.html>`__'.
  ``numGlyphs`` is set to the number of bitmap glyphs


.. _bitmaponlysfnt.X11:

X11 (Unix/Linux)
----------------

The X consortium have devised their own format which they call "OpenType Bitmap"
with extension .otb.

* The bitmap data reside in '`EBLC`_' and '`EBDT`_' tables.
* There is a zero length '`glyf`_' table
* There is a '`loca`_' table with one entry in it
* There is a '`head`_' table (not a '``bhed``')
* '`maxp`_'. ``numGlyphs`` is set to the number of bitmap glyphs, not to the
  size of the '`loca`_' table

--------------------------------------------------------------------------------

* The fonts I generate also contain the metrics tables as appropriate


.. _bitmaponlysfnt.MS:

MS
--

MicroSoft Windows provides no support for a bitmap only sfnt. So I have created
a faked up format which should work in most cases

* The bitmap data reside in '`EBLC`_' and '`EBDT`_' tables.
* There are '`glyf`_' / '`loca`_' tables with entries for every glyph. If used
  the entries will produce blank glyphs (spaces).
* There is an '`EBSC`_' table which maps common pixel sizes to the supplied
  pixel sizes. (so if a user asked for a 20 pixel strike s/he might get an 18
  pixel strike -- as opposed to getting a set of blanks.
* There is a '`head`_' table (not a '``bhed``')
* Since these fonts try to look like scalable fonts (to MS anyway) they contain
  metrics tables.

.. _bloc: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6bloc.html
.. _bdat: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6bdat.html
.. _EBLC: https://docs.microsoft.com/en-us/typography/opentype/spec/eblc
.. _EBDT: https://docs.microsoft.com/en-us/typography/opentype/spec/ebdt
.. _bhed: https://developer.apple.com/fonts/TrueType-Reference-Manual/RM06/Chap6bhed.html
.. _glyf: https://docs.microsoft.com/en-us/typography/opentype/spec/glyf
.. _loca: https://docs.microsoft.com/en-us/typography/opentype/spec/loca
.. _EBSC: https://docs.microsoft.com/en-us/typography/opentype/spec/ebsc
.. _maxp: https://docs.microsoft.com/en-us/typography/opentype/spec/maxp
.. _head: https://docs.microsoft.com/en-us/typography/opentype/spec/head