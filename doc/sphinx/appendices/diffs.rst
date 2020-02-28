Differences from `Fontographer`_
================================

* FontForge supports OpenType
* FontForge supports scripting
* FontForge understands GPOS/GSUB/GDEF tables as well as morx/prop/etc.
* The font window can display the outline font at various pixelsizes
* The font window can display a bitmap font instead (if it does then double
  clicking on a character will open a bitmap editor)
* The metrics window also lets you look at the bitmap fonts (but doesn't let you
  change their metrics, except by changing those of the outline font).
* Supports many more international standard encodings, allows you to create your
  own encodings. Better unicode support. Can generate type0 fonts for 2byte
  encodings. Support for some 94x94 CJK encodings (like JISX208)
* There's a command to jump to any particular character in the font, outline or
  bitmap windows. Characters may be entered by name, local encoding, ku ten (for
  CJK fonts), and unicode name.
* If you change the width of a character then all that character's bitmaps will
  automagically have their widths changed. Also if the character is used as a
  reference in any other character, and that other character had the same width
  then it's width will also be updated (as will the widths of any of its
  bitmaps)
* If you change the lbearing of a character (if you translate all elements in
  the foreground of an outline character, and the translation is only in the x
  direction) then all hints will also be translated, and if the character is a
  letter, then any characters which depend on it will have all other elements
  translated by the same amount. (So if you move the character "A" then the
  reference to tilde in "Ã" will be moved to correspond)

.. _diffs.accented:

* There's a command to build an accented character.

  The base characters should already have been created. Select an accented
  character (or several of them) in the font view and press
  :menuselection:`Build Accented Chars` from the :menuselection:`Element` menu.

  Suppose you select À, then Unicode says this is made up of the characters
  0x0041 and 0x0300. Now 0x0300 is not present in most postscript fonts, but it
  is sort of synonymous with either 0x02CB or 0x0060 which often are present.
  Unicode also says that 0x0300 floats on top of and centered horizontally over
  the base character. So...

  A reference to "A" will be placed into "À", and that character will be given
  the same width as "A". Then a reference to

  Some Unicode characters contain more than one accent. Additional accents will
  be treated similarly. This command can also be used to generate more general
  composite characters. Unicode 0x2163 is the roman numeral IV and this command
  may be used to build it. Greek capital Alpha looks exactly like Latin capital
  A and can be created. On the other hand the oe ligature will be replaced by an
  "o" followed by "e", so be a little careful. Some accents (for example
  cedilla) are treated unexpectedly on certain letters (different ways in
  different languages), so be careful of g-cedilla. Å often merges the ring into
  the top of the A, but here it will float above it. **Be careful.**

  .. note:: 

     My centering algorithms attempt to guess what will look centered to the
     human eye (centering it in the middle of the character will often not look
     centered). You should examine all built characters and be prepared to
     adjust the accent.

     The algorithms take some account of the italicangle.
* There's a merge font command. This is designed to allow you to build one
  monster unicode font out of several smaller fonts. It will prompt for a font
  to merge into the current one, and then will copy any characters which not in
  the current font and put them there. Note both fonts must be properly encoded.
* You can control what you want copy to copy in the font window. By default if
  you select "A" then and press copy then it will copy the outline description
  and all the bitmap descriptions for "A", so when you paste it, the bitmaps
  will follow the outline. You can choose instead just to copy from the font
  that is being displayed (which might be the outline font or might be a
  bitmap). When you Paste, whatever is in the clipboard will be pasted.

  Transform sometimes will transform bitmaps, but not all transformations map
  into bitmap transforms.
* Most changes are preserved across crashes.
* Right clicking in the character and bitmap windows will produce a menu of
  tools
* The bitmap window has a number of (not very useful) tools which fontographer
  didn't include. They are only available from the menu (not from palettes)

  * Rectangle
  * Filled Rectangle
  * Ellipse
  * Filled Ellipse
  * Flip horizontal/vertical
  * Rotate 90° Clockwise/Counter Clockwise
  * Rotate 180°

.. _Fontographer: https://en.wikipedia.org/wiki/Fontographer#Macromedia
