Data Types
==========

.. highlight:: c

.. warning::
   While much of what is described remains relevant, it has *not* been kept up
   to date with recent code changes. Cross-reference this with the current
   state of affairs in the `repository <https://github.com/fontforge/fontforge>`__.

This describes two different types of fonts, a SplineFont (which is a postscript
or truetype font) and a BDFFont (bitmap font, essentially a bdf file). There are
some similarities between the two data structures, each is basically an array
(possibly with holes in it) of characters. Each font also contains information
like the fontname.

The :ref:`SplineFont <splinefont.SplineFont>` consists of a bunch of
:ref:`SplineChars <splinefont.SplineChar>`, each containing four layers,

* A foreground layer which contains all the interesting stuff that gets put into
  the final font

  * a series of paths (called :ref:`SplineSet <splinefont.SplinePointList>`\ s or
    :ref:`SplinePointList <splinefont.SplinePointList>` sorry, I gave the data
    structure two different names)
  * a series of references to other characters (called
    :ref:`RefChar <splinefont.RefChar>`\ s)
  * a width
  * :ref:`undoes <splinefont.Undoes>`
* A background layer (which is used to allow people to trace images of characters)
  and can contain

  * a series of :ref:`SplineSet <splinefont.SplinePointList>`\ s
  * A series of :ref:`images <splinefont.ImageList>`
  * :ref:`undoes <splinefont.Undoes>`
* A grid layer, which is shared by all the characters in a font and in which you
  can put lines to indicate the Cap Height, X Height, etc. Actually you can put
  anything there but those are the obviously useful things. It will be seen in all
  characters. It contains

  * A series of :ref:`SplineSet <splinefont.SplinePointList>`\ s
  * :ref:`undoes <splinefont.Undoes>`
* A hint layer, whose format is completely different from anything else and
  consists of horizontal and vertical hints, which in turn are just a starting
  location and a width (indicating the stem. The width can be negative.)

  * A set of :ref:`hints <splinefont.Hints>`. Hints need the special tools from the
    hint menu for editing them.
  * **!!!! I do not have a way of undoing hint operations !!!!**

In multilayered mode there can be many foreground layers which can be stroked or
filled. Bitmap images may also be included.

Every SplineChar has a name associated with it, a unicode encoding and a its
original position in the font.

A SplineSet consists of a start point and an end point, and whatever splines
connect them. The start point and the end point may be the same, this either
indicates a closed path, or a degenerate path with only one point on it.

A SplinePoint contains an x,y location of the point, and the locations of the
two control points associated with that point. A control point may be coincident
with the main point. SplinePoints may connect to two splines, a previous Spline
and a next Spline. A Spline contains pointers to two SplinePoints (a start and
an end point) it also contains the parameters of the Bézier curve that those two
points describe (x = a*t^3+b*t^2+c*t+d, y=ditto). Splines are straight lines if
the two control points that are meaningful to that spline are coincident with
their respective SplinePoints. A spline contains a bit indicating whether it is
order2 (truetype quadratic format) or order3 (postscript cubic format). If it is
quadratic there is really only one control point, ff uses the convention that
the control points on the start and end SplinePoints have the same location.

A font's encoding is stored in a separate data structure called an EncMap. This
structure controls what glyph goes where and also contains a pointer to an
Encoding structure which provides a name for the encoding. The EncMap contains
two arrays, the ``map`` and the ``backmap``. The map is indexed by the
character's encoding and provides the position (in the associated splinefont's
glyph list) of the associated glyph. The backmap provides the reverse mapping,
it is indexed by a glyph's position in the splinefont and provides an encoding
for that glyph. (Note: the map array may map several encodings to one glyph, the
backmap will only indicate one of those.)

A :ref:`BDFFont <splinefont.BDFFont>` is always associated with a (possibly
empty) SplineFont and its EncMap, it consists of an array of
:ref:`BDFChars <splinefont.BDFChar>`, each containing

* a bitmap
* possibly a floating selection
* a name
* :ref:`undoes <splinefont.Undoes>`
* A pointer to the :ref:`SplineChar <splinefont.SplineChar>` with which it is
  associated

The array is ordered the same way the SplineFont's array is ordered, and the
same EncMap can be used for both.

::

   /* Copyright (C) 2000-2003 by George Williams */
   /*
    * Redistribution and use in source and binary forms, with or without
    * modification, are permitted provided that the following conditions are met:
   
    * Redistributions of source code must retain the above copyright notice, this
    * list of conditions and the following disclaimer.
   
    * Redistributions in binary form must reproduce the above copyright notice,
    * this list of conditions and the following disclaimer in the documentation
    * and/or other materials provided with the distribution.
   
    * The name of the author may not be used to endorse or promote products
    * derived from this software without specific prior written permission.
   
    * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
    * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
    * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
    * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
    * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
    * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
    * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
    */
   #ifndef _SPLINEFONT_H
   #define _SPLINEFONT_H
   
   #include "basics.h"
   #include "charset.h"
   
   enum linejoin {
       lj_miter,           /* Extend lines until they meet */
       lj_round,           /* circle centered at the join of expand radius */
       lj_bevel            /* Straight line between the ends of next and prev */
   };
   enum linecap {
       lc_butt,            /* equiv to lj_bevel, straight line extends from one side to other */
       lc_round,           /* semi-circle */
       lc_square           /* Extend lines by radius, then join them */
   };
   
   typedef struct strokeinfo {
       double radius;
       enum linejoin join;
       enum linecap cap;
       unsigned int calligraphic: 1;
       double penangle;
       double thickness;                   /* doesn't work */
   } StrokeInfo;

The above data structure is used by the ExpandStroke routines which will turn a
path into a filled shape. The information above provides the various controls
for those routines. They mean essentially what you expect them to me in
postscript.

::

   typedef struct ipoint {
       int x;
       int y;
   } IPoint;

An integer point.

::

   typedef struct basepoint {
       double x;
       double y;
   } BasePoint;

A double point. This provides the location of
:ref:`SplinePoints <splinefont.SplinePoint>` and their control points.

::

   typedef struct tpoint {
       double x;
       double y;
       double t;
   } TPoint;

A double point with a "t" value. Indicates where that location occurs on a
spline. (the start point itself is a t=0, the end point at t=1, intermediate
points have intermediate values). Used when trying to approximate new splines.

::

   typedef struct dbounds {
       double minx, maxx;
       double miny, maxy;
   } DBounds;

The bounding box of a :ref:`Spline <splinefont.Spline>`,
:ref:`SplineChar <splinefont.SplineChar>`, :ref:`RefChar <splinefont.RefChar>`,
:ref:`Image <splinefont.ImageList>`, or whatever else needs a bounding box.

.. code-block:: default
   :name: splinefont.BDFFloat

   typedef struct bdffloat {
       int16 xmin,xmax,ymin,ymax;
       int16 bytes_per_line;
       uint8 *bitmap;
   } BDFFloat;

The floating selection in a :ref:`BDFChar <splinefont.BDFChar>`.

.. code-block:: default
   :name: splinefont.Undoes

   typedef struct undoes {
       struct undoes *next;
       enum undotype { ut_none=0, ut_state, ut_tstate, ut_width,
               ut_bitmap, ut_bitmapsel, ut_composite, ut_multiple } undotype;
       union {
           struct {
               int16 width;
               int16 lbearingchange;
               struct splinepointlist *splines;
               struct refchar *refs;
               struct imagelist *images;
           } state;
           int width;
           struct {
               /*int16 width;*/    /* width should be controlled by postscript */
               int16 xmin,xmax,ymin,ymax;
               int16 bytes_per_line;
               uint8 *bitmap;
               BDFFloat *selection;
               int pixelsize;
           } bmpstate;
           struct {                /* copy contains an outline state and a set of bitmap states */
               struct undoes *state;
               struct undoes *bitmaps;
           } composite;
           struct {
               struct undoes *mult; /* copy contains several sub copies (composites, or states or widths or...) */
           } multiple;
       uint8 *bitmap;
       } u;
   } Undoes;

The undo data structure. Used by both :ref:`SplineChar <splinefont.SplineChar>`
and :ref:`BDFChar <splinefont.BDFChar>`. Also used to contain the local
clipboard. Every character layer has several Undoes (up to about 10 or so, it's
configurable) that allow it to go back several operations, these are linked
together on the next field (redos are handled similarly, of course).

Undoes come in several types, ut_none is only used by the clipboard when it is
empty.

ut_state is used by SplineChars and it contains a dump of the current state of
one layer of the character. Obviously things could be optimized a great deal
here, but this is easy. ut_tstate has the same data structure as ut_state, it is
used during transformations and is just a flag that tells the display to draw
the original as well as the currently transformed thing (so you can see what
you've done).

ut_state is also used by the clipboard when copying a SplineChar or a piece of a
SplineChar.

ut_width is used by SplineChars when the width (and nothing else) changes.

ut_bitmap is used by BDFChars and is a dump of the current state of the bitmap.
Again there is room for optimization here, but this is easy.

ut_bitmapsel is used when doing a copy of a BDFChar. If there is a selection it
(and it alone) gets copied into the selection field of the bmpstate structure.
If there is no selection the entire bitmap is converted into a
:ref:`floating selection <splinefont.BDFFloat>` and copied into the selection
field.

ut_composite is used when doing a copy from the FontView where you are copying
both the splines and the bitmaps of a character.

ut_mult is used when doing a copy from the FontView where you are copying more
than one character.

.. code-block:: default
   :name: splinefont.BDFChar

   typedef struct bdfchar {
       struct splinechar *sc;
       int16 xmin,xmax,ymin,ymax;
       int16 width;
       int16 bytes_per_line;
       uint8 *bitmap;
       int orig_pos;
       struct bitmapview *views;
       Undoes *undoes;
       Undoes *redoes;
       unsigned int changed: 1;
       unsigned int byte_data: 1;          /* for anti-aliased chars entries are grey-scale bytes not bw bits */
       BDFFloat *selection;
   } BDFChar;

The basic bitmap character structure. There's a link to the SplineChar used to
create the bitmap. Then the bounding box of the bitmap, the character's width
(in pixels of course), the number of bytes per row of the bitmap array, a
pointer to an array containing the bitmap. The bitmap is stored with 8bits
packed into a byte, the high order bit is left most. Every row begins on a new
byte boundary. There are (xmax-xmin+1) bits in each row and (xmax-xmin+8)/8
bytes in each row. There are (ymax-ymin+1) rows. A bit-value of 1 means there
bit should be drawn, 0 means it is transparent. Then the encoding in the current
font. A linked list of BitmapView structures all of which look at this bitmap
(so any changes to this bitmap need to cause a redraw in all views). A set of
undoes and redoes. A flag indicating whether the thing has been changed since it
was last saved to disk.

Up to this point I've been talking about bitmaps. It is also possible to have a
bytemap. The data structure is exactly the same, except that each pixel is
represented by a byte rather than a bit. There is a clut for this in the BDFFont
(it's the same for every character), but basically 0=>transparent, (2^n-1)
=>fully drawn, other values are shades of grey between. Can handle depths of
1,2,4 and 8 bits/pixel.

The last thing in the BDFChar is a (/an optional) floating selection. Only
present if the user has made a selection or done a paste or something like that.

.. code-block:: default
   :name: splinefont.BDFFont

   typedef struct bdffont {
       struct splinefont *sf;
       int glyphcnt;
       BDFChar **glyphs;            /* an array of charcnt entries */
       int pixelsize;
       int ascent, descent;
       struct bdffont *next;
       struct clut *clut;          /* Only for anti-aliased fonts */
   } BDFFont;

The basic bitmap font. Contains a reference to the
:ref:`SplineFont <splinefont.SplineFont>` to which it is attached. Then a size
and an array of BDFChars (note there may be NULL entries in the array if no
character is defined for that encoding). Then a temporary array which is used in
one set of routines while reencoding the font. The pixelsize of the em-square.
The ascent and descent (in pixels), these should sum to the em-square. A
character set which will match that in the SplineFont. A pointer to the next
bitmap font associated with this SplineFont.

If we are dealing with a byte font, then there will also be a clut. This
contains a count of the number of entries in the array, and then the array
itself. Currently the number of entries here is always 16, but that could
change.

.. code-block:: default
   :name: splinefont.SplinePoint

   enum pointtype { pt_curve, pt_corner, pt_tangent };
   typedef struct splinepoint {
       BasePoint me;
       BasePoint nextcp;          /* control point */
       BasePoint prevcp;          /* control point */
       unsigned int nonextcp:1;
       unsigned int noprevcp:1;
       unsigned int nextcpdef:1;
       unsigned int prevcpdef:1;
       unsigned int selected:1;    /* for UI */
       unsigned int pointtype:2;
       unsigned int isintersection: 1;
       uint16 flex;                /* This is a flex serif have to go through icky flex output */
       struct spline *next;
       struct spline *prev;
   } SplinePoint;

A SplinePoint is located at the position specified by "me". The control point
associated with the next :ref:`Spline <splinefont.Spline>` is positioned at
nextcp, while that associated with the previous Spline is at prevcp. Then there
are a couple of flags that simplify tests. If the nextcp is degenerate (ie. at
the same place as me) then nonextcp is set, similarly for prevcp. If the user
has not touched the control points then they will have their default values, and
when the user moves the point around fontforge will update the control points
appropriately, if they do not have default values then fontforge will only
offset them.

If the point is selected then that bit will be set.

Every point is classified as a curve point, a corner point and a tangent point.

The isintersection bit is used internally to the SplineOverlap routines. The
flex value is for flex hints. It is read in from a type1 font and then ignored.
Someday I may use it.

Finally we have pointers to the two Splines than connect to this point.

::

   typedef struct linelist {
       IPoint here;
       struct linelist *next;
   } LineList;
   
   typedef struct linearapprox {
       double scale;
       unsigned int oneline: 1;
       unsigned int onepoint: 1;
       struct linelist *lines;
       struct linearapprox *next;
   } LinearApprox;

These are the lines used to approximate a :ref:`Spline <splinefont.Spline>` when
drawing it. They are cached so they don't need to be regenerated each time.
There's a different set of lines for every scale (as there is a different amount
of visible detail). They get freed and regenerated if the Spline changes.

.. code-block:: default
   :name: splinefont.Spline

   typedef struct spline1d {
       double a, b, c, d;
   } Spline1D;
   
   typedef struct spline {
       unsigned int islinear: 1;
       unsigned int isticked: 1;
       unsigned int isneeded: 1;
       unsigned int isunneeded: 1;
       unsigned int ishorvert: 1;
       unsigned int order2: 1;
       SplinePoint *from, *to;
       Spline1D splines[2];                /* splines[0] is the x spline, splines[1] is y */
       struct linearapprox *approx;
       /* Possible optimizations:
           Precalculate bounding box
           Precalculate points of inflection
       */
   } Spline;

A spline runs from the ``from`` :ref:`SplinePoint <splinefont.SplinePoint>` to
the ``to`` SplinePoint. If both control points of a Spline are degenerate, then
the Spline is linear (actually there are some other case that will lead to
linear splines, sometimes these will be detected and turned into the canonical
case, other times they will not be). The remaining bits are used when processing
Splines in various functions. Most are used in the SplineOverlap routines, but
some are used in other places too.

The Spline1D structures give the equations for the x and y coordinates
respectively (splines[0] is for x, splines[1] is for y).

.. code-block:: default
   :name: splinefont.SplinePointList

   typedef struct splinepointlist {
       SplinePoint *first, *last;
       struct splinepointlist *next;
   } SplinePointList, SplineSet;

A SplinePointList (or a SplineSet) is a collection of
:ref:`Spline <splinefont.Spline>`\ s and
:ref:`SplinePoint <splinefont.SplinePoint>`\ s. Every SplinePoint can connect to
two Splines (next and prev). Every Spline connects to two SplinePoints (from and
to). A SplinePointList is a connected path. There are three cases:

* ``first != last`` which means that we have an open path. Here first->prev==NULL
  and last->next==NULL.
* ``first == last`` and :menuselection:`first --> prev==NULL` which means we have
  a degenerate path consisting of a single point, and last->next==NULL as well
* ``first == last`` and :menuselection:`first --> prev!=NULL` which means we have
  a closed path. This should be the most common case.

Generally a series of paths will make up a character, and they are linked
together on the next field.

.. code-block:: default
   :name: splinefont.RefChar

   typedef struct refchar {
       int16 adobe_enc, orig_pos
       int unicode_enc;            /* used by paste */
       double transform[6];        /* transformation matrix (first 2 rows of a 3x3 matrix, missing row is 0,0,1) */
       SplinePointList *splines;
       struct refchar *next;
       unsigned int checked: 1;
       unsigned int selected: 1;
       DBounds bb;
       struct splinechar *sc;
   } RefChar;

A :ref:`SplineChar <splinefont.SplineChar>` may contain a reference to another
character. For instance "Agrave" might contain a reference to an "A" and a
"grave". There are three different encoding values, of which orig_pos is not
always up to date. Adobe_enc gives the location in the Adobe Standard Encoding
which is used by the seac command in type1 fonts. If this is -1 then the
character isn't in the adobe encoding and it won't be possible to put a
reference to it into a Type1 font (truetype doesn't have this restriction, it
has other ones). The transformation matrix is a standard postscript
transformation matrix (3 rows of 2 columns. First 2 rows provide standard
rotation/scaling/flipping/skewing/... transformations, last row provides for
translations. (Both postscript and truetype have restrictions on what kinds of
transformations are acceptable). The splines field provides a quick way of
drawing the referred character, it is the result of applying the transformation
matrix on all splines in the referred character. There may be several referred
characters and they are linked together on the next field. The checked field is
used to insure that we don't have any loops (ie. on characters which refer to
themselves). The selected field indicates that the reference is selected. The bb
field provides a transformed bounding box. And the sc field points to the
SplineChar we are referring to.

.. code-block:: default
   :name: splinefont.KernPair

   typedef struct kernpair {
       struct splinechar *sc;
       int off;
       struct kernpair *next;
   } KernPair;

If a character should be kerned with another, then this structure provides that
info. Every SplineChar has a linked list of KernPairs attached to it. In that
list the sc field indicates the other character in the pair, and off defines the
offset between them (or rather the difference from what their respective left
and right bearings would lead you to believe it should be). Next points to the
next kernpair.

.. code-block:: default
   :name: splinefont.Hints

   typedef struct hints {
       double base, width;
       double b1, b2, e1, e2;
       double ab, ae;
       unsigned int adjustb: 1;
       unsigned int adjuste: 1;
       struct hints *next;
   } Hints;

The only important fields here are base, width and next. The others are used
temporarily by the SplineFill routines. Base gives the location (in either x or
y space) of where the stem starts, and width is how long it is. Width may be
negative (in which case base is where the stem ends). Next points to the next
hint for the character.

.. code-block:: default
   :name: splinefont.ImageList

   typedef struct imagelist {
       struct gimage *image;
       double xoff, yoff;          /* position in character space of upper left corner of image */
       double xscale, yscale;      /* scale to convert one pixel of image to one unit of character space */
       DBounds bb;
       struct imagelist *next;
       unsigned int selected: 1;
   } ImageList;

SplineChars may have images in their backgrounds (or foregrounds for multilayer
characters). This structure contains a pointer to the image to be displayed, an
indication of where it should be positioned, and how it should be scaled (I do
not support any other transformations on images). The bounding box is after the
transformations have been applied. The next field points to the next image, and
selected indicates whether this one is selected or not.

.. code-block:: default
   :name: splinefont.SplineChar

   typedef struct splinechar {
       char *name;
       int enc, unicodeenc;
       int width;
       int16 lsidebearing;         /* only used when reading in a type1 font */
       int16 ttf_glyph;            /* only used when writing out a ttf font */
       SplinePointList *splines;
       Hints *hstem;          /* hstem hints have a vertical offset but run horizontally */
       Hints *vstem;          /* vstem hints have a horizontal offset but run vertically */
       RefChar *refs;
       struct charview *views;   /* All CharViews that look at us */
       struct splinefont *parent;
       unsigned int changed: 1;
       unsigned int changedsincelasthhinted: 1;
       unsigned int changedsincelastvhinted: 1;
       unsigned int manualhints: 1;
       unsigned int ticked: 1;     /* For reference character processing */
       unsigned int changed_since_autosave: 1;
       unsigned int widthset: 1;   /* needed so an emspace char doesn't disappear */
       struct splinecharlist { struct splinechar *sc; struct splinecharlist *next;} *dependents;
               /* The dependents list is a list of all characters which reference*/
               /*  the current character directly */
       SplinePointList *backgroundsplines;
       ImageList *backimages;
       Undoes *undoes[2];
       Undoes *redoes[2];
       KernPair *kerns;
       uint8 *origtype1;           /* The original type1 (unencoded) character string */
       int origlen;                /*  its length. These get freed once the user has changed */
                                   /*  the character. Until then the preserve things like */
                                   /*  hint substitution which we wouldn't otherwise understand */
   } SplineChar;

Every character has a name (sometimes it is ".notdef" but it's there). A
location (encoding) in the current font, a unicode code point (which may be -1
if not in unicode). Every character has a width. The next two fields are only
meaningful when loading or saving a font in the appropriate format. lbearing is
needed to handle seac commands, and the ttf_glyph is needed for just about
everything when writing a ttf font. The splines field gives all the foreground
paths (:ref:`SplinePointLists <splinefont.SplinePointList>`). Hints for
horizontal and vertical stems. A set of other characters referenced in this one,
again only in the foreground. Then a linked list of all CharViews displaying this
SplineChar (if this guy changes, all must be updated to reflect the change). A pointer
to the :ref:`SplineFont <splinefont.SplineFont>` that contains us. A set of bits:
changed means the character has changed since the last save to disk,
changedsincelasthhinted means that we need to run autohint on the horizontal
stems, changedsincelastvhinted for vertical stems. Manual hints means the user
has taken control of providing hints, and we should only run autohint if
explicitly asked to. Ticked is a temporary field usually to avoid infinite loops
of referred characters. changed_since_autosave indicates that the next time we
update our autosave database we should write this character to it. Widthset
means the user has changed the width. If we didn't have this bit we might think
that an em space had nothing in it (instead of having an em-space in it).

If a SplineChar is referred to in another character, then when we change the
original we must also update anything that refers to it (if we change A, we must
also redisplay Agrave).

Then backgroundsplines represent the SplinePointLists in the background layer,
and backimages represent the images in the background layers.

Undoes[0] contains undoes for the foreground and undoes[1] contains undoes for
the background. Same for redoes.

Finally kerns provides a list of kerning data for any special characters that
follow this one. For instance the combination "VA" might need kerning, then the
SplineChar representing "V" would have a pointer to a
:ref:`KernPair <splinefont.KernPair>` with data on "A".

.. code-block:: default
   :name: splinefont.SplineFont

   typedef struct splinefont {
       char *fontname, *fullname, *familyname, *weight;
       char *copyright;
       char *filename;
       char *version;
       double italicangle, upos, uwidth;
       int ascent, descent;
       int glyphcnt;
       SplineChar **glyphs;
       unsigned int changed: 1;
       unsigned int changed_since_autosave: 1;
       unsigned int display_antialias: 1;
       unsigned int dotlesswarn: 1;                /* User warned that font doesn't have a dotless i character */
       /*unsigned int wasbinary: 1;*/
       struct fontview *fv;
       enum charset encoding_name;
       SplinePointList *gridsplines;
       Undoes *gundoes, *gredoes;
       BDFFont *bitmaps;
       char *origname;             /* filename of font file (ie. if not an sfd) */
       char *autosavename;
       int display_size;
   } SplineFont;

The first four names are taken straight out of PostScript, as are copyright,
version, italicangle, underlinepos, underlineweight. Ascent and descent together
sum to make the em-square. Normally this will be 1000, but you can change that
if you want.

Charcnt says how big the chars array will be. There may be holes (ie. NULL
values) in the array.

Changed indicates that some character, bitmap, metric, something has changed
since we last saved the file. changed_since_autosave means that something has
changed since autosave last happened (so we should actually process this font
the next time autosave rolls around).

Display_antialias means we are displaying an antialias bytemap font in the
FontView, rather than a bitmap font. These look better but are slower.

Dotlesswarn means that we've warned the user when s/he attempted to create an
accented character based on i or j and a dotless version of those characters was
not present in the font (there's no point in warning him again. The operation
proceeded with a dotted version).

There is only one FontView associated with a font (other
data structures allowed for multiple views, but the font does not).

The font has a characterset and an encoding.

Gridsplines contains the :ref:`SplinePointLists <splinefont.SplinePointList>`
for all the splines displayed as background grids for characters. And gundoes
and gredoes are the un/redoes associated with them.

Bitmaps contains all the bitmaps that have been generated for this SplineFont.

Origname contains the name of the file we read in to get this one. Filename
above contains the name of an .sfd file associated with this font, but origname
can also contain random postscript or truetype fonts.

Autosavename indicates the name currently being used to save crash recovery
stuff to disk.

Display_size indicates how big we'd like the FontView to display our font.


Function declarations
---------------------

::

   struct fontdict;         /* the next four data structures are in psfont.h */
   struct chars;
   struct findsel;
   struct charprocs;
   enum charset;                   /* in charset.h */
   
   extern SplineFont *SplineFontFromPSFont(struct fontdict *fd);
   extern int SFOneWidth(SplineFont *sf);
   extern struct chars *SplineFont2Chrs(SplineFont *sf, int round);
   enum fontformat { ff_pfa, ff_pfb, ff_ptype3, ff_ptype0, ff_ttf, ff_none };
   extern int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format);
   extern int WriteTTFFont(char *fontname,SplineFont *sf);
   int SFReencodeFont(SplineFont *sf,enum charset new_map);
   
   extern int DoubleNear(double a,double b);
   extern int DoubleNearish(double a,double b);
   
   extern void LineListFree(LineList *ll);
   extern void LinearApproxFree(LinearApprox *la);
   extern void SplineFree(Spline *spline);
   extern void SplinePointFree(SplinePoint *sp);
   extern void SplinePointListFree(SplinePointList *spl);
   extern void SplinePointListsFree(SplinePointList *head);
   extern void RefCharFree(RefChar *ref);
   extern void RefCharsFree(RefChar *ref);
   extern void UndoesFree(Undoes *undo);
   extern void HintsFree(Hints *h);
   extern Hints *HintsCopy(Hints *h);
   extern SplineChar *SplineCharCopy(SplineChar *sc);
   extern BDFChar *BDFCharCopy(BDFChar *bc);
   extern void ImageListsFree(ImageList *imgs);
   extern void SplineCharFree(SplineChar *sc);
   extern void SplineFontFree(SplineFont *sf);
   extern void SplineRefigure(Spline *spline);
   extern Spline *SplineMake(SplinePoint *from, SplinePoint *to);
   extern LinearApprox *SplineApproximate(Spline *spline, double scale);
   extern int SplinePointListIsClockwise(SplineSet *spl);
   extern void SplineSetFindBounds(SplinePointList *spl, DBounds *bounds);
   extern void SplineCharFindBounds(SplineChar *sc,DBounds *bounds);
   extern void SplineFontFindBounds(SplineFont *sf,DBounds *bounds);
   extern void SplinePointCategorize(SplinePoint *sp);
   extern void SCCategorizePoints(SplineChar *sc);
   extern SplinePointList *SplinePointListCopy(SplinePointList *base);
   extern SplinePointList *SplinePointListCopySelected(SplinePointList *base);
   extern SplinePointList *SplinePointListTransform(SplinePointList *base, double transform[6], int allpoints );
   extern SplinePointList *SplinePointListShift(SplinePointList *base, double xoff, int allpoints );
   extern SplinePointList *SplinePointListRemoveSelected(SplinePointList *base);
   extern void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase);
   extern void SplinePointListSelect(SplinePointList *spl,int sel);
   extern void SCRefToSplines(SplineChar *sc,RefChar *rf);
   extern void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf);
   extern void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc);
   extern void SCRemoveDependent(SplineChar *dependent,RefChar *rf);
   extern void SCRemoveDependents(SplineChar *dependent);
   extern RefChar *SCCanonicalRefs(SplineChar *sc, int isps);
   extern void BCCompressBitmap(BDFChar *bdfc);
   extern void BCRegularizeBitmap(BDFChar *bdfc);
   extern void BCPasteInto(BDFChar *bc,BDFChar *rbc,int ixoff,int iyoff, int invert);
   extern BDFChar *SplineCharRasterize(SplineChar *sc, int pixelsize);
   extern BDFFont *SplineFontRasterize(SplineFont *sf, int pixelsize);
   extern BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize,int linear_scale);
   extern BDFFont *SplineFontAntiAlias(SplineFont *sf, int pixelsize,int linear_scale);
   extern void BDFCharFree(BDFChar *bdfc);
   extern void BDFFontFree(BDFFont *bdf);
   extern void BDFFontDump(char *filename,BDFFont *font, char *encodingname);
   extern double SplineSolve(Spline1D *sp, double tmin, double tmax, double sought_y, double err);
   extern void SplineFindInflections(Spline1D *sp, double *_t1, double *_t2 );
   extern int NearSpline(struct findsel *fs, Spline *spline);
   extern double SplineNearPoint(Spline *spline, BasePoint *bp, double fudge);
   extern void SCMakeDependent(SplineChar *dependent,SplineChar *base);
   extern SplinePoint *SplineBisect(Spline *spline, double t);
   extern Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
           TPoint *mid, int cnt);
   extern int SplineIsLinear(Spline *spline);
   extern int SplineInSplineSet(Spline *spline, SplineSet *spl);
   extern void SplineCharMerge(SplineSet **head);
   extern void SplineCharSimplify(SplineSet *head);
   extern void SplineCharRemoveTiny(SplineSet *head);
   extern SplineFont *SplineFontNew(void);
   extern SplineFont *SplineFontBlank(int enc,int charcnt);
   extern SplineSet *SplineSetReverse(SplineSet *spl);
   extern SplineSet *SplineSetsExtractOpen(SplineSet **tbase);
   extern SplineSet *SplineSetsCorrect(SplineSet *base);
   extern void SPAverageCps(SplinePoint *sp);
   extern void SplineCharDefaultPrevCP(SplinePoint *base, SplinePoint *prev);
   extern void SplineCharDefaultNextCP(SplinePoint *base, SplinePoint *next);
   extern void SplineCharTangentNextCP(SplinePoint *sp);
   extern void SplineCharTangentPrevCP(SplinePoint *sp);
   extern int PointListIsSelected(SplinePointList *spl);
   extern void SplineSetsUntick(SplineSet *spl);
   extern void SFOrderBitmapList(SplineFont *sf);
   
   extern SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si);
   extern SplineSet *SplineSetRemoveOverlap(SplineSet *base);
   
   extern void FindBlues( SplineFont *sf, double blues[14], double otherblues[10]);
   extern void FindHStems( SplineFont *sf, double snaps[12], double cnt[12]);
   extern void FindVStems( SplineFont *sf, double snaps[12], double cnt[12]);
   extern void SplineCharAutoHint( SplineChar *sc);
   extern void SplineFontAutoHint( SplineFont *sf);
   extern int AfmSplineFont(FILE *afm, SplineFont *sf,int type0);
   extern char *EncodingName(int map);
   
   extern int SFDWrite(char *filename,SplineFont *sf);
   extern int SFDWriteBak(SplineFont *sf);
   extern SplineFont *SFDRead(char *filename);
   extern SplineFont *SFReadTTF(char *filename);
   extern SplineFont *LoadSplineFont(char *filename);
   extern SplineFont *ReadSplineFont(char *filename);      /* Don't use this, use LoadSF instead */
   
   extern SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,int i);
   extern SplineChar *SFMakeChar(SplineFont *sf,int i);
   extern BDFChar *BDFMakeChar(BDFFont *bdf,int i);
   
   extern void SCUndoSetLBearingChange(SplineChar *sc,int lb);
   extern Undoes *SCPreserveState(SplineChar *sc);
   extern Undoes *SCPreserveWidth(SplineChar *sc);
   extern Undoes *BCPreserveState(BDFChar *bc);
   extern void BCDoRedo(BDFChar *bc,struct fontview *fv);
   extern void BCDoUndo(BDFChar *bc,struct fontview *fv);
   
   extern int SFIsCompositBuildable(SplineFont *sf,int unicodeenc);
   extern void SCBuildComposit(SplineFont *sf, SplineChar *sc, int copybmp,
           struct fontview *fv);
   
   extern void BDFFontDump(char *filename,BDFFont *font, char *encodingname);
   extern int getAdobeEnc(char *name);
   
   extern SplinePointList *SplinePointListInterpretPS(FILE *ps);
   extern void PSFontInterpretPS(FILE *ps,struct charprocs *cp);
   
   extern int SFFindChar(SplineFont *sf, int unienc, char *name );
   
   extern char *getFontForgeDir(char *buffer);
   extern void DoAutoSaves(void);
   extern int DoAutoRecovery(void);
   extern SplineFont *SFRecoverFile(char *autosavename);
   extern void SFAutoSave(SplineFont *sf);
   extern void SFClearAutoSave(SplineFont *sf);
   #endif
