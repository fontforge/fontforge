The X11 PCF bitmap font file format
===================================

This file is based on the
`X11 sources <http://ftp.x.org/pub/R6.4/xc/lib/font/bitmap/>`__. If something
here disagrees with what is found in the sources, my statement is incorrect. The
sources are definitive.

The pcf (portable compiled format) file format is a binary representation of a
bitmap font used by the XServer. It consists of a file header followed by a
series of tables, with the header containing pointers to all the tables.


File Header
-----------

The file header contains 32 bit integers stored with the least significant byte
first.

.. highlight:: c

::

   char header[4];                  /* always "\1fcp" */
   lsbint32 table_count;
   struct toc_entry {
       lsbint32 type;              /* See below, indicates which table */
       lsbint32 format;            /* See below, indicates how the data are formatted in the table */
       lsbint32 size;              /* In bytes */
       lsbint32 offset;            /* from start of file */
   } tables[table_count];

The type field may be one of:

::

   #define PCF_PROPERTIES               (1<<0)
   #define PCF_ACCELERATORS            (1<<1)
   #define PCF_METRICS                 (1<<2)
   #define PCF_BITMAPS                 (1<<3)
   #define PCF_INK_METRICS             (1<<4)
   #define PCF_BDF_ENCODINGS           (1<<5)
   #define PCF_SWIDTHS                 (1<<6)
   #define PCF_GLYPH_NAMES             (1<<7)
   #define PCF_BDF_ACCELERATORS        (1<<8)

The format field may be one of:

::

   #define PCF_DEFAULT_FORMAT       0x00000000
   #define PCF_INKBOUNDS           0x00000200
   #define PCF_ACCEL_W_INKBOUNDS   0x00000100
   #define PCF_COMPRESSED_METRICS  0x00000100

The format field may be modified by:

::

   #define PCF_GLYPH_PAD_MASK       (3<<0)            /* See the bitmap table for explanation */
   #define PCF_BYTE_MASK           (1<<2)            /* If set then Most Sig Byte First */
   #define PCF_BIT_MASK            (1<<3)            /* If set then Most Sig Bit First */
   #define PCF_SCAN_UNIT_MASK      (3<<4)            /* See the bitmap table for explanation */

The format will be repeated as the first word in each table. Most tables only
have one format (default), but some have alternates. The high order three bytes
of the format describe the gross format. The way integers and bits are stored
can be altered by adding one of the mask bits above.

All tables begin on a 32bit boundary (and will be padded with zeroes).


Properties Table
----------------

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   int32 nprops;                   /* Stored in whatever byte order is specified in the format */
   struct props {
       int32 name_offset;          /* Offset into the following string table */
       int8 isStringProp;
       int32 value;                /* The value for integer props, the offset for string props */
   } props[nprops];
   char padding[(nprops&3)==0?0:(4-(nprops&3))];   /* pad to next int32 boundary */
   int string_size;                /* total size of all strings (including their terminating NULs) */
   char strings[string_size];
   char padding2[];

These properties are the Font Atoms that X provides to users. Many are described
in
`xc/doc/specs/XLFD/xlfd.tbl.ms <http://ftp.x.org/pub/R6.4/xc/doc/specs/XLFD/xlfd.tbl.ms>`__
or
`here <http://rpmfind.net/linux/RPM/redhat/6.2/i386/XFree86-doc-3.3.6-20.i386.html>`__
(the X protocol does not limit these atoms so others could be defined for some
fonts).

To find the name of a property: ``strings + props[i].name_offset``


.. _pcf-format.MetricsData:

Metrics Data
------------

Several of the tables (PCF_METRICS, PCF_INK_METRICS, and within the accelerator
tables) contain metrics data which may be in either compressed
(PCF_COMPRESSED_METRICS) or uncompressed (DEFAULT) formats. The compressed
format uses bytes to contain values, while the uncompressed uses shorts. The
(compressed) bytes are unsigned bytes which are offset by 0x80 (so the actual
value will be ``(getc(pcf_file)-0x80)``. The data are stored as:

Compressed

::

   uint8 left_sided_bearing;
   uint8 right_side_bearing;
   uint8 character_width;
   uint8 character_ascent;
   uint8 character_descent;
    /* Implied character attributes field = 0 */

Uncompressed

::

   int16 left_sided_bearing;
   int16 right_side_bearing;
   int16 character_width;
   int16 character_ascent;
   int16 character_descent;
   uint16 character_attributes;

This provides the data needed for an XCharStruct.


Accelerator Tables
------------------

These data provide various bits of information about the font as a whole. This
data structure is used by two tables PCF_ACCELERATORS and PCF_BDF_ACCELERATORS.
The tables may either be in DEFAULT format or in PCF_ACCEL_W_INKBOUNDS (in which
case they will have some extra metrics data at the end.

The accelerator tables look like:

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   uint8 noOverlap;                /* if for all i, max(metrics[i].rightSideBearing - metrics[i].characterWidth) */
                                   /*      <= minbounds.leftSideBearing */
   uint8 constantMetrics;          /* Means the perchar field of the XFontStruct can be NULL */
   uint8 terminalFont;             /* constantMetrics true and forall characters: */
                                   /*      the left side bearing==0 */
                                   /*      the right side bearing== the character's width */
                                   /*      the character's ascent==the font's ascent */
                                   /*      the character's descent==the font's descent */
   uint8 constantWidth;            /* monospace font like courier */
   uint8 inkInside;                /* Means that all inked bits are within the rectangle with x between [0,charwidth] */
                                   /*  and y between [-descent,ascent]. So no ink overlaps another char when drawing */
   uint8 inkMetrics;               /* true if the ink metrics differ from the metrics somewhere */
   uint8 drawDirection;            /* 0=>left to right, 1=>right to left */
   uint8 padding;
   int32 fontAscent;               /* byte order as specified in format */
   int32 fontDescent;
   int32 maxOverlap;               /* ??? */
   Uncompressed_Metrics minbounds;
   Uncompressed_Metrics maxbounds;
   /* If format is PCF_ACCEL_W_INKBOUNDS then include the following fields */
       Uncompressed_Metrics ink_minbounds;
       Uncompressed_Metrics ink_maxbounds;
   /* Otherwise those fields are not in the file and should be filled by duplicating min/maxbounds above */

BDF Accelerators should be preferred to plain Accelerators if both tables are
present. BDF Accelerators contain data that refers only to the encoded
characters in the font (while the simple Accelerator table includes all glyphs),
therefore the BDF Accelerators are more accurate.


Metrics Tables
--------------

There are two different metrics tables, PCF_METRICS and PCF_INK_METRICS, the
former contains the size of the stored bitmaps, while the latter contains the
minimum bounding box. The two may contain the same data, but many CJK fonts pad
the bitmaps so all bitmaps are the same size. The table format may be either
DEFAULT or PCF_COMPRESSED_METRICS (see the section on
:ref:`Metrics Data <pcf-format.MetricsData>` for an explanation).

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   /* if the format is compressed */
       int16 metrics_count;
       Compressed_Metrics metrics[metrics_count];
   /* else if format is default (uncompressed) */
       int32 metrics_count;
       Uncompressed_Metrics metrics[metrics_count];
   /* endif */


The Bitmap Table
----------------

The bitmap table has type PCF_BITMAPS. Its format must be PCF_DEFAULT.

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   int32 glyph_count;              /* byte ordering depends on format, should be the same as the metrics count */
   int32 offsets[glyph_count];     /* byte offsets to bitmap data */
   int32 bitmapSizes[4];           /* the size the bitmap data will take up depending on various padding options */
                                   /*  which one is actually used in the file is given by (format&3) */
   char bitmap_data[bitmapsizes[format&3]];    /* the bitmap data. format contains flags that indicate: */
                                   /* the byte order (format&4 => LSByte first)*/
                                   /* the bit order (format&8 => LSBit first) */
                                   /* how each row in each glyph's bitmap is padded (format&3) */
                                   /*  0=>bytes, 1=>shorts, 2=>ints */
                                   /* what the bits are stored in (bytes, shorts, ints) (format>>4)&3 */
                                   /*  0=>bytes, 1=>shorts, 2=>ints */


The Encoding Table
------------------

The encoding table has type PCF_BDF_ENCODINGS. Its format must be PCF_DEFAULT.

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   int16 min_char_or_byte2;        /* As in XFontStruct */
   int16 max_char_or_byte2;        /* As in XFontStruct */
   int16 min_byte1;                /* As in XFontStruct */
   int16 max_byte1;                /* As in XFontStruct */
   int16 default_char;             /* As in XFontStruct */
   int16 glyphindeces[(max_char_or_byte2-min_char_or_byte2+1)*(max_byte1-min_byte1+1)];
                                   /* Gives the glyph index that corresponds to each encoding value */
                                   /* a value of 0xffff means no glyph for that encoding */

For single byte encodings min_byte1==max_byte1==0, and encoded values are
between [min_char_or_byte2,max_char_or_byte2]. The glyph index corresponding to
an encoding is glyphindex[encoding-min_char_or_byte2].

Otherwise [min_byte1,max_byte1] specifies the range allowed for the first (high
order) byte of a two byte encoding, while [min_char_or_byte2,max_char_or_byte2]
is the range of the second byte. The glyph index corresponding to a double byte
encoding (enc1,enc2) is
glyph_index[(enc1-min_byte1)*(max_char_or_byte2-min_char_or_byte2+1)+
enc2-min_char_or_byte2].

Not all glyphs need to be encoded. Not all encodings need to be associated with
glyphs.


The Scalable Widths Table
-------------------------

The encoding table has type PCF_SWIDTHS. Its format must be PCF_DEFAULT.

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   int32 glyph_count;              /* byte ordering depends on format, should be the same as the metrics count */
   int32 swidths[glyph_count];     /* byte offsets to bitmap data */

The scalable width of a character is the width of the corresponding postscript
character in em-units (1/1000ths of an em).


The Glyph Names Table
---------------------

The encoding table has type PCF_GLYPH_NAMES. Its format must be PCF_DEFAULT.

::

   lsbint32 format;                 /* Always stored with least significant byte first! */
   int32 glyph_count;              /* byte ordering depends on format, should be the same as the metrics count */
   int32 offsets[glyph_count];     /* byte offsets to string data */
   int32 string_size;
   char string[string_size];

The postscript name associated with each character.

--------------------------------------------------------------------------------

Other sources of info:

* http://www.tsg.ne.jp/GANA/S/pcf2bdf/pcf.pdf
* http://myhome.hananet.net/~bumchul/xfont/pcf.txt
