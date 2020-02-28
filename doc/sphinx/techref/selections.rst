X Selections and the X Clipboard
================================

The general description of how selections and the clipboard works under X is
given in
`clipboards.txt <http://www.freedesktop.org/standards/clipboards.txt>`__.
FontForge tries to follow this.

FontForge only supports the PRIMARY selection within textfields. Text is
exported as

* ``STRING``
* ``UTF8_STRING``
* ``text/plain;charset=UTF8``
* ``text/plain;charset=ISO-10646-USC2``

Traditionally the PRIMARY selection is available for pasting by clicking on the
middle mouse button.

After a Copy command FontForge will place data in the X CLIPBOARD. For text
selections (in textfields) the above format types are supported. For contour
data (ie. the contents of an outline character) the format "image/eps" or
"image/svg" will be used. As a special case, if a single point is in the
clipboard then it will also be exported as a STRING containing the coordinates
of the point (to make it easier for people to identify points when sending me
email, or in the comment field). For bitmap image data either "image/bmp" or
"image/png" (assuming you have libpng on your system).