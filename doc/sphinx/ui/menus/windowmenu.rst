The Window Menu
===============

.. _windowmenu.Outline:

.. object:: Open Outline

   In the font view this will open outline views on all selected glyphs (if
   there are more than 15 or so it will ask whether you really meant to do
   that).

   In the bitmap view it will open an outline view on the current glyph.

   In the metrics view it will open an outline view on whatever glyph is active.

   This menu item is always grey in an outline view.

   Note: It is possible to have more than one window displaying the same glyph.
   Any editing that occurs in one should be reflected in all.

.. _windowmenu.Bitmap:

.. object:: Open Bitmap

   In the font view this will open bitmap views on all selected glyphs (if there
   are more than 15 or so it will ask whether you really meant to do that). If
   the font view is displaying a bitmap then it will open a bitmap view showing
   that pixelsize, otherwise it will pick a pixelsize.

   In the outline view it will open a bitmap view on the current glyph. It will
   pick a pointsize.

   In the metrics view it will open an outline view on whatever glyph is active,
   as with the font view, if it is displaying a bitmap it will use its
   pixelsize, otherwise it will pick a pixelsize.

   This menu item is always grey in an bitmap view.

   Note: It is possible to have more than one window displaying the same glyph.
   Any editing that occurs in one should be reflected in all.

.. _windowmenu.Metrics:

.. object:: Open Metrics

   In the font view it will open a metrics view displaying all selected glyphs
   (the order in which you selected those glyphs is important). The new metrics
   view will display whatever the font view displays (outline or bitmap).

   In the outline view it will open a metrics view displaying the current glyph.
   The display will be an outline.

   In the bitmap view it will open a metrics view displaying the current glyph.
   The display will be a bitmap in the current size.

   This menu item is always grey in the metrics view.

   It is possible to have more than one metrics view open at a time.

.. _windowmenu.window:

After that the window menu is just a list of all the open windows. If the window
has been changed (and not saved since) then it has a white background. The names
of font views are in red. Views connected to a given font will all be displayed
together (with the font view on top). Clicking on an entry raises that window.
