fontimage
=========

Generate a font thumbnail image ::

   fontimage [--o outfile] [--width num] [--height num] [--pixelsize num] [--fontname] [--text string] [--version] [--help] fontfile

*Fontimage* produces a thumbnail image of a font (which may be in any format
:doc:`fontforge </index>` can read).

You may either explicitly specify text to be displayed in the font, or rely on
fontimage's default behavior, which is to check for various scripts it knows
about and display something appropriate for each one present.

.. program:: fontimage

.. option:: --help

   Displays a list of arguments and descriptions of each.

.. option:: --width num

   Specifies the width (in pixels) of the output image. If omitted the image
   will be as wide as it needs to be to display the text.

.. option:: --height num

   Specifies the height (in pixels) of the output image. If omitted the image
   will be as high as it needs to be to display the text.

.. option:: --o outputfile

   Specifies the name of the output image file. The format is determined by the
   extension. Currently only ".bmp" and ".png" are supported. If omitted,
   fontimage will choose a filename based on the fontname of the font.

.. option:: --pixelsize num

   Specifies the pixelsize at which to display text. This may be specified
   multiple times. Each specification applies to any --text arguments which
   occur after it.

.. option:: --fontname

   Include the fontname as a line of text.

.. option:: --text string

   Specifies a string to be displayed. The string *must* be in UTF-8. This
   option may be provided multiple times to supply more than one line of text.
   If omitted entirely, fontimage will examine the scripts it knows about in the
   font and will display text appropriate to each one present.

.. option:: --version

   Displays the fontimage version.


See Also
--------

:doc:`fontforge </index>`