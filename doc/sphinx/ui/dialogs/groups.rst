Groups of Glyphs
================

A group is a collection of glyphs. It may be any collection that appeals to you.
My expectation is that groups will be used to collect glyphs which are visually
associated (in some way) close together so that they may be examined and changed
without the visual clutter of the rest of the font. So you might make a group
that consisted of "b d p q" which often have similar shapes, or in a Korean font
you might wish to look at all sylables with a give cho-sung.

.. image:: /images/fontview-grouped.png
   :align: left

FontForge treats groups like baby encodings. In the image at left, it is
displaying a font showing only the glyphs "b d p q". The order the glyphs are
mentioned in the group is the order in which they will be displayed in the
fontview -- and, like an encoding the group may be compaced so there are no
empty holes for missing glyphs (so if the group had contained "b d p uniAC00 q"
and the font was missing uniAC00, the display would be exactly the same) or
uncompacted with holes.

Glyphs in a group may be specified by glyph name, by unicode code point or by a
range of unicode values. The examples above all used glyph names. A unicode code
point looks like "U+XXXX" (where XXXX indicates a hex number). Occasionally
fonts will use this notation to name their glyphs (they should not, but some
do), that should not be a significant problem because such glyphs should have
the obvious unicode code points and so will still match. A range of unicode code
points looks like "U+XXXX-U+YYYY" (XXXX < YYYY).

A groups are hierarchical. A group may either contain a list of glyphs, or a
list of sub-groups. (If you display (in the fontview) a group which contains
subgroups you will see all glyphs in all sub-groups. A glyph may occur more than
once if that is desirable. So a group could contain "A A A A" and the fontview
would show four copies of "A" (I'm not sure why you'd want to do that, but you
could). As you design your groups you can ask fontforge to check so that you do
not inadvertently put the same glyph in more that one group (or in the same
group twice).


Display by Groups
-----------------

.. image:: /images/displaygroups.png
   :align: right

The Display by Groups dialog is shown in the screen shot at right. The top-level
group is called "Groups" it has two children, one called "Vowels" which is a
terminal group and contains the glyph names of the latin vowels, the other
called "consonants" which itself contains three subgroups.

Any group with children will have (to the right of its name) a square box
containing either a "+" or a "-", clicking on this box will either expand all
the sub-groups of the group, or contract them into invisibility (here all are
expanded).

Clicking on a group's name will select it (clicking again will deselect it).
Here groups "ascenders" and "descenders" are selected. If you were to press
``[OK]`` then the fontview would show the glyphs associated with "b d f h k l t
g j p q y" in that order.

Underneath everything is a check box you may use to specify whether you want the
results compacted or not.


Define Groups
-------------

.. image:: /images/definegroups.png
   :align: right

This dialog is a bit more complicated than the previous one. It again contains
the group list at the top of the dialog, but here only one group at a time may
be selected, and information about it is displayed in the bottom part of the
dialog and is subject to change.

You can ``[Delete]`` the group (this also deletes any subgroups). If the group
contains no glyphs then you may add sub-groups to it. Every group has a name. If
the group contains no subgroups you may add glyphs to it.

The ``[Set]`` and ``[Select]`` buttons interact with the selection in the
associated fontview. ``[Set]`` sets the glyph list to the glyphs selected in the
fontview, while ``[Select]`` changes the selection to the glyphs specified in
the glyph list. Normally glyphs are entered into the glyph list by ``<*>Name``,
but you can change the radio button to ``<*>Unicode`` and glyphs will be entered
as U+0067 U+006A U+0070-U+0071 U+0079.

If you select ``[*]No glyph duplicates`` then ff will prevent you from entering
the same glyph name (or unicode code point) twice (ff will allow you to enter
"A" and U+0041 -- because I'm lazy and this is harder to check for and less
likely to occur). If you set this in a parent group then there can be no glyph
duplicates among any of its children.
