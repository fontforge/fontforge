Non standard extensions used FontForge in True/Open Type
========================================================


Non standard feature tags
-------------------------

Internal

* ``' REQ'`` -- tag used for required features

For TeX

I no longer use these. MS has come up with a MATH table which can contain all
the TeX information.

* ``ITLC`` -- :ref:`Italic correction <PfaEdit-TeX.Italic>`

  I have proposed this as an extension to OpenType. There was some discussion of
  it, but I believe the idea died.
* ``TCHL`` -- :ref:`TeX Charlist <PfaEdit-TeX.charlist>`
* ``TEXL`` -- :ref:`TeX Extension List <PfaEdit-TeX.extension>`


Non standard tables.
--------------------

* :ref:`'PfEd' the FontForge extensions table <non-standard.PfEd>`
* :ref:`'TeX ' the TeX metrics table <non-standard.TeX>`
* :ref:`'BDF ' the BDF properties table <non-standard.BDF>`
* :ref:`'FFTM' the FontForge time stamp table <non-standard.FFTM>`

--------------------------------------------------------------------------------

* (:doc:`standard tables <TrueOpenTables>`)

Non standard file formats

* :ref:`Bitmap only sfnt for MS Windows <bitmaponlysfnt.MS>`


.. _non-standard.PfEd:

'PfEd' -- the FontForge extensions table
----------------------------------------

(the name is 'PfEd' for historical reasons)

   The table begins with a table header containing a version number and a count
   of sub-tables

   .. list-table::
      :widths: 15 15 70

      * - uint32
        - version
        - currently 0x00010000
      * - uint32
        - count
        -

   This is followed by a table of contents, there will be count replications of
   the following structure (ie. a tag and offset for each sub-table

   .. list-table::
      :widths: 15 15 70

      * - uint32
        - tag
        -
      * - uint32
        - offset
        - from start of 'PfEd' table

   The format of the subtable depends on the sub-table's tag. There are
   currently 3 tags supported, these are

   * ':ref:`colr <non-standard.colr>`' -- per glyph color sub-table (stores the
     color that appears in the font view)
   * ':ref:`cmnt <non-standard.cmnt>`' -- per glyph comment sub-table (stores the
     comment that appears in the Char Info dialog for the character)
   * ':ref:`fcmt <non-standard.fcmt>`' -- the font comment sub-table (stores the
     comment that appears in the Font Info dialog)
   * ':ref:`flog <non-standard.fcmt>`' -- the font log sub-table (looks just like
     the 'fcmt' subtable)
   * ':ref:`cvtc <non-standard.cvtc>`' -- the cvt comments subtable
   * ':ref:`GPOS <non-standard.GPOS>`' -- Save lookup, lookup subtable and anchor
     class names of GPOS lookups
   * ':ref:`GSUB <non-standard.GPOS>`' -- Save lookup and lookup subtable names of
     GSUB lookups
   * ':ref:`guid <non-standard.guid>`' -- Save guideline locations
   * ':ref:`layr <non-standard.layr>`' -- Save background and spiro layers


.. _non-standard.colr:

'colr' -- the per-glyph color sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of ranges

   .. list-table::

      * - uint16
        - version
        - 0
      * - uint16
        - count
        - of ranges

   After this will be <count> instances of the following structure

   .. list-table::

      * - uint16
        - starting glyph index
        -
      * - uint16
        - ending glyph index
        -
      * - uint32
        - color
        - expressed as a 24bit rgb value


.. _non-standard.cmnt:

'cmnt' -- the per-glyph comment sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of ranges

   .. list-table::

      * - uint16
        - version
        - 0/1
      * - uint16
        - count
        - of ranges

   After this will be <count> instances of the following structure

   .. list-table::

      * - uint16
        - starting glyph index
        -
      * - uint16
        - ending glyph index
        -
      * - uint32
        - offset
        - from the start of this sub-table

   The offset points to an array of offsets (<end>-<start>+1+1) elements in the
   array, so one element for each glyph index mentioned in the range structure
   above, with one left over which allows readers to compute the length of the
   last string.

   .. list-table::

      * - uint32
        - offset
        - from start of table
      * -
        - ...
        -

   And each of these offsets points to a unicode (UCS2 for version 0, UTF-8 for
   version 1) string. The strings are assumed to be consecutive, so the length
   of each may be calculated by subtracting its offset from the offset to the
   next string.


.. _non-standard.fcmt:

'fcmt' -- the font comment (and FONTLOG) sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of ranges

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - version
        - 0/1
      * - uint16
        - length
        - number of characters in the string

   In version 0 this is followed by <length> Unicode (UCS2) characters. In
   version 1 it is followed by <length> bytes in utf8 encoding.


.. _non-standard.cvtc:

'cvtc' -- the cvt comments sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of cvt entries
   which might have comments

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - version
        - 0
      * - uint16
        - count
        - number of entries in the cvt comments array

          which might be smaller than the number of entries in the cvt array itself
          if we want to save space
      * - uint16
        - offset[count]
        - offsets to the start of utf8-encoded, NUL terminated strings. Or 0 if this
          cvt entry has no comment


.. _non-standard.GPOS:

'GPOS/GSUB' -- lookup names
^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of ranges

   .. list-table::
      :widths: 20 20 60

      * - uint16
        - version
        - 0
      * - uint16
        - count
        - of lookups in this table

   Then there will be an array of count elements (one for each lookup, ordered
   as the lookups are in the GPOS or GSUB table)

   .. list-table::
      :widths: 20 20 60

      * - uint16
        - offset
        - to lookup name
      * - uint16
        - offset
        - to lookup subtable structure

   These offsets are based on the start of the subtable. The name offset points
   to a NUL terminated UTF-8 encoded string. The subtable offset points to the
   following structure:

   .. list-table::
      :widths: 20 20 60

      * - uint16
        - count
        - of lookup subtables in this lookup

   Then there will be an array of count elements (one for each subtable, ordered
   as the subtables are in the lookup of the GPOS or GSUB table)

   .. list-table::
      :widths: 20 20 60

      * - uint16
        - offset
        - to lookup subtable name
      * - uint16
        - offset
        - to anchor class structure

   These offsets are also based on the start of the subtable. The name offset
   points to a NUL terminated UTF-8 encoded string. The anchor offset may be 0
   if this subtable doesn't have any anchor classes, otherwise it points to the
   following structure:

   .. list-table::
      :widths: 20 20 60

      * - uint16
        - count
        - of anchor classes in this lookupsubtable

   Then there will be an array of count elements (one for each anchor class)

   .. list-table::
      :widths: 20 20 60

      * - uint16
        - offset
        - to anchor class name


.. _non-standard.guid:

'guid' -- guidelines
^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of ranges

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - version
        - 1
      * - uint16
        - vcount
        - number of vertical guidelines
      * - uint16
        - hcount
        - number of horizontal guidelines
      * - uint16
        - mbz
        - At some point this may contain info on diagonal guidelines. For now it is
          undefined
      * - uint16
        - offset
        - To a full description of the guideline layer

   I provide the guidelines in two formats. Either may be omitted. The first
   format simply describes the horizontal and vertical lines used as guidelines.
   The second format provides a full description of all contours (curved,
   straight, horizontal or diagonal) which fontforge uses. I provide both since
   most apps seem to have a simpler guideline layer than ff does.

   This table is followed by two arrays, one for vertical guidelines, one for
   horizontal guides. Both arrays have the same element type (except that the
   position is for a different coordinate in the horizontal/vertical cases)

   .. list-table::
      :widths: 15 15 70

      * - int16
        - position
        - x location of vertical guides, y location of horizontal ones
      * - uint16
        - offset
        - to name, a NUL terminated utf8 string. (offset may be 0 if guideline is
          unnamed).

   The offset to the guideline layer points to a variable length structure which
   is also used in the :ref:`'layr' subtable <non-standard.layr>`


.. _non-standard.glyph-layer:

'glyph-layer' -- Data structure representing all the contours of a layer of a
glyph
"""""

   The sub-table header begins with a version number, and a count of ranges

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - count
        - of contours
      * - uint16
        - count
        - of references (not present in version 0 layers)
      * - uint16
        - mbz
        - reserved for a count of images

   This is followed by an array of structures describing each contour

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - offset
        - to start of contour data
      * - uint16
        - offset
        - to a name for the contour, a utf8, NUL terminated string (or 0 if the
          contour is unnamed)

   All offsets from the start of the glyph-layer structure.

   This is followed by an array of structures describing each reference

   .. list-table::
      :widths: 15 15 70

      * - fixed16.16
        - transform[6]
        - A PostScript transformaton matrix where each member is a signed 4 byte
          integers which should be divided by 32768.0 to allow for non-integral
          values
      * - uint16
        - gid
        - The Glyph ID of the glyph being referred to

   Contour data live in a variable length block. It's basic idea is that it is a
   list of <command>, <data> pairs. Each command is a byte which consists of two
   parts, a verb which specifies what happens (and how many items of data are
   needed) and a modifier which specifies how each data item is represented. The
   verbs are postscript-like drawing operations: moveto, lineto, curveto, (and
   quadratic curveto), close path, etc. There are also separate verbs for
   specifying spiro control points -- these are just the standard spiro type
   bytes ('v', 'o', 'c', '[' and ']'), no modifier is applied to the spiro
   commands, their data are always 2 coordinates in fixed notation.

   The low order two bits of the command (except for the spiro and close
   commands) specify the data format:

   .. list-table::
      :widths: 15 85

      * - 0
        - signed byte data for values -128 to 127
      * - 1
        - signed short data for values -32768 to 32767
      * - 2
        - A signed 4 byte integer which should be divided by 256.0 for non-integral
          coordinates (or for big ones)
      * - 3
        - Undefined and erroneous for now

   Each command will start at the current point and draw to the point specified
   by its data. The data are relative to the last point specified (except for
   moveto, which is absolute, there being on previous point).

   The verb may be one of the following:

   .. list-table::
      :widths: 15 85

      * - 0
        - MoveTo, takes 2 coordinates (x,y). This must begin each contour and may
          not appear elsewhere within it
      * - 4
        - LineTo, also takes 2 coordinates
      * - 8
        - HLineTo, draws a horizontal line, so only the new x coordinate need be
          specified.
      * - 12
        - VLineTo, draws a vertical line, so only the new y coordinate need be
          specified.
      * - 16
        - QCurveTo, takes one off-curve control point and one on-curve point, 4
          coordinates total, to draw a quadratic bezier spline
      * - 20
        - QImplicit, only specifies the control point. The on-curve point will be
          the average of the control point specified here, and the one specified in
          the next QCurveTo or Q*Implicit command.
      * - 24
        - QHImplicit, Same as above, except only the x coordinate of the new control
          point is specified
      * - 28
        - QVImplicit, Same as above except only the y coordinate of the new control
          point is specified.
      * - 32
        - CurveTo, takes two off-curve control point and one on-curve point, 6
          coordinates total, to draw a cubic bezier spline
      * - 36
        - VHCurveTo, The first control point is vertical from the current point, so
          only its y coordinate is specified. The final point is horizontal from the
          last control point so only its x coordinate is specified. 4 coordinates
          total y1, x2,y2, x3.
      * - 40
        - HVCurveTo, Reverse of the above x1, x2,y2, y3
      * - 44
        - Close, No data. Closes (and ends) the current contour. Will draw a line
          from the start point to the end point if needed.
      * - 45
        - End, No data. Ends the current contour, but leaves it open.

   These are basically the drawing operators in the type1 charstrings. If my
   terse descriptions make no sense look there for a more complete description.

   .. rubric:: examples

   suppose we want to draw a box (0,0)->(0,200)->(200,200)->(200,0)->(0,0). Then
   the glyph-layer would look like:

   .. list-table::
      :widths: 10 15 10 65

      * - Header
        - one contour
        - (ushort) 1
        -
      * - Header
        - no images
        - (ushort) 0
        -
      * - First Contour
        - offset to data
        - (ushort) 8
        - The number of bytes from the start of the glyph-layer to the Contour Data
      * - First Contour
        - no name
        - (ushort) 0
        -
      * - Contour Data
        - Move To 0,0
        - (byte)0, (byte)0, (byte)0
        - Both coordinates are <127 and can fit in a byte, so the modifier is 0. The
          command is also 0, and the coordinates are each 0
      * - Contour Data
        - VLine To [0,]200
        - (byte)(12+1) (short)200
        - Vertical motion => VLineTo. Only the new y value need be specified. Is
          relative to the last position, but that was 0
      * - Contour Data
        - HLine To 200[,200]
        - (byte)(8+1) (short)200
        - Horizontal motion => HLineTo. Only the new x value need be specified. Is
          relative to the last position, but that was 0
      * - Contour Data
        - VLine To [200,]0
        - (byte)(12+1) (short)-200
        - Vertical motion => HLineTo. Only the new y value need be specified. We
          move from 200 to 0, so the relative change is -200
      * - Contour Data
        - Close
        - (byte)44
        - We can draw the final line by closing the path


.. _non-standard.layr:

'layr' -- Background layer data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - version
        - 1
      * - uint16
        - count
        - number of layers in this sub-table

   This is followed by an array of structures describing each layer

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - typeflag
        - Low order byte is the type

          2=>quadratic splines, 3=>cubic splines, 1=>spiros, other values not
          defined

          High order byte are the flags

          0x100 => foreground layer
      * - uint16
        - offset
        - to the name of this layer, a utf8, NUL terminated string
      * - uint32
        - offset
        - to the data for this layer

   The layer data is a block of ranges specifying which glyphs (by GID) have
   data for this layer. (the type field is present so that applications can
   ignore layers which they do not support).

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - count
        - of ranges

   This is followed by an array of structures one for each range:

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - start
        - first GID in the range
      * - uint16
        - last
        - last GID in the range
      * - uint32
        - offset
        - to an array of offsets, one for each GID in the range. The offsets in this
          array may be 0. These offsets in turn point to a
          :ref:`glyph-layer structure <non-standard.glyph-layer>`

   All offsets are relative to the start of the 'layr' subtable.


.. _non-standard.TeX:

'TeX ' -- the TeX metrics table
-------------------------------

   The table begins with a table header containing a version number and a count
   of sub-tables

   .. list-table::
      :widths: 15 15 70

      * - uint32
        - version
        - currently 0x00010000
      * - uint32
        - count
        -

   This is followed by a table of contents, there will be count replications of
   the following structure (ie. a tag and offset for each sub-table

   .. list-table::
      :widths: 15 15 70

      * - uint32
        - tag
        -
      * - uint32
        - offset
        - from start of 'PfEd' table

   The format of the subtable depends on the sub-table's tag. There are
   currently 3 tags supported, these are

   * ':ref:`ftpm <non-standard.ftpm>`' -- the font parameter table
   * ':ref:`htdp <non-standard.htdp>`' -- per glyph height/depth sub-table (stores
     TeX's idea of the height and depth of glyphs (should correct for optical
     overshoot))
   * ':ref:`sbsp <non-standard.sbsp>`' -- per glyph sub/super-script sub-table


.. _non-standard.htdp:

'htdp' -- the per-glyph height/depth sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of glyphs

   .. list-table::

      * - uint16
        - version
        - 0
      * - uint16
        - count
        - of glyphs

   After this will be <count> instances of the following structure

   .. list-table::

      * - uint16
        - height
        - in em-units
      * - uint16
        - depth
        -

I store these values in em-units rather than in the fix_word of a tfm file
because em-units make more sense in a sfnt and take up less space.


.. _non-standard.sbsp:

'sbsp' -- the per-glyph sub/super-script sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   This sub-table has essentially the same format as the previous one. The
   sub-table header begins with a version number, and a count of glyphs

   .. list-table::

      * - uint16
        - version
        - 0
      * - uint16
        - count
        - of glyphs

   After this will be <count> instances of the following structure

   .. list-table::

      * - uint16
        - subscript offset
        - in em-units
      * - uint16
        - superscript offset
        -

I store these values in em-units rather than in the fix_word of a tfm file
because em-units make more sense in a sfnt and take up less space.


.. _non-standard.ftpm:

'ftpm' -- the font parameter (font dimensions) sub-table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

   The sub-table header begins with a version number, and a count of parameters

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - version
        - 0
      * - uint16
        - count
        - number of parameters in the font

   And this is followed by <count> instances of the following structure:

   .. list-table::

      * - uint32
        - tag
        - parameter name
      * - int32
        - value
        -

   I store these values as fix_words and *not* as em-units because their meaning
   is not constrained by the spec and the ``Slant`` parameter (at the least) can
   not be converted to em-units.

   I have defined the following 4-letter parameter tags

   .. list-table::
      :header-rows: 1

      * - Tag
        - Meaning
        - traditional font parameter number in tfm file (font dimension number)
      * - Slnt
        - Slant
        - 1
      * - Spac
        - Space
        - 2
      * - Stre
        - Stretch
        - 3
      * - Shnk
        - Shrink
        - 4
      * - XHgt
        - XHeight
        - 5
      * - Quad
        - Quad
        - 6
      * - ExSp
        - Extra Space
        - 7 (in text fonts)
      * - MtSp
        - Math Space
        - 7 (in math and math extension fonts)
      * - Num1
        - Num1
        - 8 (in math fonts)
      * - Num2
        - Num2
        - 9
      * - Num3
        - Num2
        - 10
      * - Dnm1
        - Denom1
        - 11
      * - Dnm2
        - Denom2
        - 12
      * - Sup1
        - Sup1
        - 13
      * - Sup2
        - Sup2
        - 14
      * - Sup3
        - Sup3
        - 15
      * - Sub1
        - Sub1
        - 16
      * - Sub2
        - Sub2
        - 17
      * - SpDp
        - Sup Drop
        - 18
      * - SbDp
        - Sub Drop
        - 19
      * - Dlm1
        - Delim 1
        - 20
      * - Dlm2
        - Delim 2
        - 21
      * - AxHt
        - Axis height
        - 22
      * - RlTk
        - Default Rule Thickness
        - 8 (in math extension fonts)
      * - BOS1
        - Big Op Spacing1
        - 9
      * - BOS2
        - Big Op Spacing2
        - 10
      * - BOS3
        - Big Op Spacing3
        - 11
      * - BOS4
        - Big Op Spacing4
        - 12
      * - BOS5
        - Big Op Spacing5
        - 13


.. _non-standard.BDF:

'BDF ' -- the BDF properties table
----------------------------------

   The table begins with a table header containing a version number and a count
   of strikes

   .. list-table::
      :widths: 15 15 70

      * - uint16
        - version
        - currently 0x0001
      * - uint16
        - strike-count
        -
      * - uint32
        - offset to string table
        - (from start of BDF table)

   This is followed by an entry for each strike identifying how many properties
   that strike has.

   .. list-table::

      * - uint16
        - ppem
        -
      * - uint16
        - property-count
        -

   Then there will be the properties, first there with be property-count[1]
   properties from the first strike, then property-count[2] properties for the
   second, etc. Each property looks like:

   .. list-table::
      :widths: 15 15 70

      * - uint32
        - name
        - offset into the string table of the property's name
      * - uint16
        - type
        - 0=>string

          1=>atom

          2=>int

          3=>uint

          0x10 may be ored into one of the above types to indicate a real property
      * - uint32
        - value
        - For strings and atoms, this is an offset into the string table

          for integers it is the value itself

   The string table is a series of ASCII bytes. Each string is NUL terminated.


.. _non-standard.FFTM:

'FFTM' the FontForge time stamp table
-------------------------------------

   The table begins with a table header containing a version number and is
   followed by a series of timestamps (same format as the timestamps in the head
   table -- 64 bit times, seconds since 00:00:00, 1-Jan-1904).

   I don't think this is a duplication of the information in the 'head' table.
   Neither the Apple nor OpenType spec is clear: Does head.creationtime refer to
   the creation time of the truetype/opentype file, or of the font itself. After
   examining various fonts of Apple's, it appears that the 'head' entries
   contain the dates for the font file and not the font. The times in this table
   are specifically the creation time of the font (the sfd file), while the
   times in the 'head' table contain the creation time of the truetype or
   opentype font file.

   .. list-table::
      :widths: 15 15 70

      * - uint32
        - version
        - currently 0x00000001
      * - int64
        - FontForge's own timestamp
        - (the date of the sources for fontforge)
      * - int64
        - creation date of this font
        - Not the creation date of the tt/ot file,

          but the date the sfd file was created.

          (not always accurate).
      * - int64
        - last modification date of this font
        - Not the modification date of the file,

          but the time a glyph, etc. was last

          changed in the font database.

          (not always accurate)
