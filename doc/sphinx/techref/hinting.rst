Hinting
=======

PostScript originally required that hints should not overlap. Now it requires
that any active set of hints be non-overlapping, but it allows you to change
sets as you move through the glyph.

So to implement hinting with hint substitution FontForge needs to know not only
the position and width of the stem being hinted, but also where the stem should
be active. And it needs to know what stems overlap which other stems.

.. image:: /images/Hints.png
   :align: left

The horizontal stems are drawn in light green. The vertical stems are drawn in
light blue. The areas where the stems are active are filled in as rectangles,
outside of that area the stems' outlines are drawn with dotted lines. Most of
these stems do not conflict with any other stems, but two, the bottom two
horizontal stems conflict with each other. All the non-overlapping stem hints
may be active at the same time, but only one of the two conflicting stem hints
may be active. As FontForge outputs the glyph it decides at each point what
stems need to be active, and activates them.

PostScript now allows FontForge to control relative counter sizes. Counters are
the spaces between adjacent stems. In the example at left there are two counters
of equal sizes between the horizontal stems on the left, and three counters of
equal sizes on the right. These form two independent counter groups, and
FontForge's :menuselection:`Hints --> Auto Counter` command will generate a
counter description for each of them. You may also control the counters directly
by using the Counters pane of the :menuselection:`Element --> Char Info` dialog.

Counters spaces appear between vertical stems too, and in this case FontForge
would output one counter group for the vertical stems.


Serifs
------

.. image:: /images/serif.png
   :align: right

One of the major advantages of hint substitution is that it allows certain
standard serif situations to be described. In the example at right, the serif
one the left is smaller than the stem a bit further right.

.. image:: /images/serif2.png
   :align: left

But it doesn't solve all problems (or I don't know how to use it to do so).
Consider the serif on the right. Hint substitution would allow us to define a
hint for the main vertical stem and for the short vertical stem that corresponds
to the serif. But the problem that needs to be solved is making the distance
between the left edge of the serif and the left edge of the stem be the same as
the distance between the right edge of the stem and the right edge of the serif.
But doing so would require overlapping hints, active at the same time.
:small:`I think`


Automatic Hinting
-----------------

FontForge's AutoHint command can be used to figure out horizontal and vertical
stem hints. It will also find hint substitution points (if any are needed), and
will check for the simple counter description allowed by OpenType for non-Asian
fonts (the equivalent of h/vstem3 hints in type1 fonts).

Whenever you change a glyph FontForge keeps track of that fact that the hints
are out of date (when you autohint a glyph, or make a manual change to the hints
FontForge resets this flag). If you generate a postscript font from something
that contains glyphs with out of date hints, then FontForge will automagically
regenerate them.

This may not always be appropriate. You can turn this off in general with the
:ref:`AutoHint preference item <prefs.AutoHint>`. You may turn it off for an
individual glyph with the :menuselection:`Hints --> Don't AutoHint` command.


Manual Hinting
--------------

I hope that the FontForge's autohint command will be good enough that manual
efforts will not be needed. But hopes like this are rarely fulfilled.

The AutoHint command should find all stems in a glyph. Some of them it deems to
be useless and will remove them. If you find that FontForge is removing a hint
that you think is important then invoke :menuselection:`Hint --> AutoHint` with
the shift key held down, this will tell FontForge not to prune any of the stems.
You may then use Review Hints to prune things yourself.

If you find that you must create your own hints, FontForge will only allow you
to specify the position and width of the hint, it will then go off and try to
guess the extents where the hint should be active.

You may manually:

* Clear all horizontal stem hints
* Clear all vertical stem hints
* Select two points and create a vertical stem hint that whose width is horizontal
  distance between the two points
* Select two points and create a horizontal stem hint whose width is the vertical
  distance between the two points
* Create a horizontal hint starting at an arbitrary y location with an arbitrary
  vertical width.
* Create a vertical hint starting at an arbitrary x location with an arbitrary
  horizontal width.
* Review all your hints, manually changing or removing any (or all).

Once you have made a manual change to a glyph's hints with any of the above
commands, that glyph will be marked "Don't AutoHint" until you explicitly call
the AutoHint command on it. You may also explicitly use the
:menuselection:`Hints --> Don't AutoHint` command to turn off FontForge's
automatic hinting attempts.

You may also manually

* Set hint change-over points
* Set counter groups for a glyph

These do not set the "Don't AutoHint" bit.


Manual Hint Substitution Points
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

FontForge's :menuselection:`Hints --> Hint Substitution Pts` command will
automatically figure out where hint substitution should occur, but if this isn't
good enough you may do it manually with the
:ref:`Element->Get Info <getinfo.HintMask>` command (when the selection is a
single point to which you want to attach a hintmask).

If a glyph has conflicting hints then the first point in the glyph (the first
point on the first contour) MUST have a hintmask attached to it. If it does not,
the FontForge will automatically figure substitution points when it generates
the font. Other points are not required (but may) have a hintmask.

The hintmask specifies what hints should be active when that point is
positioned, and will control all subsequent points until a new hintmask is
established. So in the following example, the selected point (on left) has the
highlight hints active at it, In the glyph view the currently active stems are
drawn much darker than the inactive ones. Points with hintmasks attached to
them have circles drawn around them.

.. flex-grid::

  * - .. image:: /images/charwithhintmask.png
    - .. image:: /images/hintmaskinfo.png

Remember to hold the control key down when making disjoint selections.

.. _hinting.Counter:

Manual Counter Groups
^^^^^^^^^^^^^^^^^^^^^

FontForge also has a :menuselection:`Hints --> Auto Counter` command which tries
to figure out what stems form a counter group.

Again you can do this manually, with the :ref:`Counter <charinfo.CounterMasks>`
pane of the :menuselection:`Element --> Char Info` dialog. When creating a new
counter group you are presented with a dialog very similar to the one above in
which you must select a set of hints that bound a set of counters.


The Private dictionary
^^^^^^^^^^^^^^^^^^^^^^

This contains font-wide hinting infomation. Things like: The standard width of a
stem in the font (StdVW, StdHW), The standard height of capitals and lower case
letters (BlueValues, OtherBlues), and so forth. These live in the PostScript
Private dictionary. Once you have hinted your font fontforge can generate
reasonable values for these settings (But you must have hinted the font first!)
-- See the [Guess] button in the
:ref:`Element->Font Info->PS Private <fontinfo.Private>` dialog. Then once you
have generated BlueValues you should rehint the font (the hints depend on the
BlueValues, just as the BlueValues depend on the hints:-).


Hints and previously existing fonts.
------------------------------------

FontForge will not be able to convert TrueType instructions into PostScript
hints when it reads a TrueType font (the format is too complex, there are too
many possibilities, instead FontForge stores all the truetype instructions and
writes them back out uninterpretted).

From a Type1 font it will happily read all the hints in a glyph, and keep track
of hint substitution points. It will not read counter hints though.

In an OpenType (Type2) font FontForge will read in all the hints, keep track of
hint substitution points and the counter hints.


Hinting and TrueType
--------------------

FontForge can auto-instruct a truetype font
(:menuselection:`Hints --> AutoInstr`). FontForge also allows you the somewhat
arcane practice of editing glyph instruction programs directly
(:menuselection:`Hints --> Edit Instructions`).

FontForge's Auto Instructor bases its output on the PostScript Stem Hints, the
contents of the Private dictionary, and diagonal stems. You can create diagonal
stems manually (:menuselection:`Hints --> Add DHint`), or you can let the
autohinter do this for you
(:menuselection:`File --> Preferences --> PSHints --> DetectDiagonalStems`).

Please look at the :doc:`page on instructing fonts </ui/dialogs/ttfinstrs>`.


Hint questions:
---------------

I find the hint documentation inadequate for me to do a good job at hinting.

* Section 2.4 of
  `T1_Supp.pdf <http://partners.adobe.com/asn/developer/pdfs/tn/5015.Type1_Supp.pdf>`__
  says that vertical counters are offset from the lbearing, but the example 2.6
  shows them being offset from 0.
* In `Type2 <http://partners.adobe.com/asn/developer/pdfs/tn/5177.Type2.pdf>`__ is
  it ok to use Counter mask if LanguageGroup is not 1 and the stems don't fit into
  a \*stem3 pattern? Or can cntrmask only be used for \*stem3 in latin letters?
* .. image:: /images/NoPointHint.png
     :align: right

  How do hints work? Is a hint meaningful if there are no points associated with
  it? As in the "O" at right, where the two vertical stems have no points
  associated with them.
* Is it meaningful for a hint to have points only on one side of it? Can
  reasonable hints be written for the serif cap above left?

:ref:`Overview of Hinting in PostScript and TrueType <overview.Hints>`.

:doc:`Hinting menu. </ui/menus/hintsmenu>`