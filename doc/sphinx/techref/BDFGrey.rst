Extensions to Adobe's BDF [#c1]_ for greymap fonts
==================================================

Adobe has defined a file format for distributing bitmap fonts called BDF.
MicroSoft extended this format to support greymap (anti-aliased, each pixel
representing various levels of grey rather than just black/white) fonts.
Recently others have started using this same format
(`gbdfed <http://math.nmsu.edu/~mleisher/Software/gbdfed/>`_,
`freetype <http://freetype.sf.net/>`_ and :doc:`fontforge </index>`).

In Section 3.1 of the format description Adobe defines a "SIZE" keyword.
MicroSoft has added an optional fourth parameter to this keyword

.. parsed-literal::

   SIZE *PointSize XRes YRes [Bits_Per_Pixel]*

This fourth parameter may take the values 1, 2, 4 and 8. If omitted it is
assumed to be 1. (FontForge will also read in fonts with values of 16 and 32,
but it will simply ignore any low order data and only retain the high order
byte).

FontForge will also include a font property

.. parsed-literal::

   BITS_PER_PIXEL *Value*

in all greymap fonts it generates.

FontForge will also mark all such fonts as version 2.3 of the bdf standard ::

   STARTFONT 2.3

The bitmap data are stored with 8, 4, 2 or 1 pixels packed into a byte depending
on whether BITS_PER_PIXEL is 1, 2, 4 or 8. As in Adobe's spec all data are
presented in hex and an even number of nibbles must be present.

**Example:**

::

   BITMAP
   C8
   ENDCHAR

Would represent the pixel sequences

.. list-table:: 
   :header-rows: 1
   :stub-columns: 1

   * - Bits/Pixel
     - Pixels
   * - 1
     - 1,1,0,0,1,0,0,0
   * - 2
     - 3,0,2,0
   * - 4
     - C,8
   * - 8
     - C8

.. rubric:: Footnotes

.. [#c1] http://partners.adobe.com/asn/developer/PDFS/TN/5005.BDF_Spec.pdf
