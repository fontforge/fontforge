Autotracing bitmaps in FontForge
================================

FontForge does not have a native autotrace, but it will happily use the output
of two freely available programs which do autotracing. These are:

* Peter Selinger's `potrace <http://potrace.sf.net/>`_
* Martin Weber's `autotrace program <http://sourceforge.net/projects/autotrace/>`_

  .. note:: 

     Autotrace 2.8 changed its argument conventions (around Dec 2001). New
     versions (after 15 Dec) of fontforge will not work with autotrace2.7, old
     versions of fontforge will not work with autotrace2.8. I see no way to make
     fontforge work with both or to detect the current version...

  .. note:: 

     I may not be loading the results of autotrace properly in all cases (I do in
     my test cases, of course). AutoTrace traces out both foreground and
     background regions, so I may sometimes leave behind a lump which represents a
     background area. Just delete it if it happens (and send me the image so I can
     fix things up).

You must download at least one of these, (and possibly build it), and install it
somewhere along your PATH.

Having done that you must get an image into the background of the glyph(s) you
want to autotrace. There are several ways of doing this:

.. object:: From a bitmap font

   If you want to autotrace a bitmap font then (from the FontView)

   * (You will probably want to start out with a new font, but you might not)
   * :menuselection:`File --> Import`
   * select the bitmap font type (bdf, FON, embedded in a ttf file, TeX bitmap
     (GF, PK), etc.)
   * turn on the ``[*] As Background`` flag
   * Select your font file
   * This should place the bitmaps into the background of the glyphs in the font.
     Nothing will be visible in the font view, but if you open up an outline glyph
     view, you should see the bitmap version of the glyph as a grey background.

.. object:: From the clipboard

   If you have an application that supports sending image selections by mime
   type (kde does this), then you should be able to copy the image in that
   application and paste it into the appropriate glyph window in FontForge

.. object:: From an image file

   If you have a bitmap in an image (it works best if it IS a black and white
   bitmap image, rather than a color image)

   * Open up an outline glyph view for the appropriate glyph
   * Make the Background layer active (this is usually not necessary)
   * :menuselection:`File --> Import`
   * select Format=Image (this will show you any image format that fontforge
     supports)
   * Select your file
   * The image should now be visible in the background of this glyph

.. object:: From multiple image files

   If you have many images, you can load them with one command, but you must
   name them appropriately. For example if your font contains unicode characters
   U+0041 (LATIN CAPITAL LETTER A) through U+0049 (LATIN CAPITAL LETTER J) then
   create files called "uni0041.png", "uni0042.png", ... "uni0049.png"
   containing the images for the appropriate characters, then (from the Font
   View)

   * Select the requisite glyphs
   * :menuselection:`File --> Import`
   * select Format=Image template
   * select the first of your images, "uni0041.png"
   * This should load all images that match that template ("uni*.png") into the
     appropriate glyph slot

Once you have background images in your font (and have installed an autotrace
program)

* Select all the glyphs you wish to autotrace
* :menuselection:`Element --> Autotrace`

This can take a while, so be patient.

.. note:: 

   Unless you are working with a TeX bitmap font, you will most likely have an
   extremely low resolution image. Autotrace programs work better the more
   resolution you give them.

If you hold down the shift key when you invoke AutoTrace from the menu then you
will be prompted for arguments to pass to it, if you do not hold down the shift
key FontForge will use the same arguments it used last time. AutoTrace's
arguments are described in "$ autotrace -help" or in the README file that came
with the program. Please do not specify input/output files or formats. FontForge
fills these in.