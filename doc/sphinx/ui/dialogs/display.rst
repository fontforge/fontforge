Display & Print Fonts
=====================

This dialog allows you to view and print various samples of any fonts you have
loaded into FontForge and see how they look on the screen and together.

.. image:: /images/SampleText.png
   :align: right

The UI is based on a standard OS-specific print dialog, with the addition of a
Preview pane which allows you to select and customize the printed sample.

There are several printing options, which can be choosed with radio button next
to the preview area. 

.. _printing.sample_text:

Sample Text
^^^^^^^^^^^

The sample text can be edited in the text area, which is visible right away in
Windows, or appears when clicking on a one-line preview of the sample text.

.. image:: /images/SampleTextEditor.png
   :align: right

The sample text can be formatted to have different weights, slant, sizes and
widths. The availability of these options depends on the fonts from the same
family being currently open. For example, if you have a regular, bold and
italic version of the same font open, you can change the weight and slant of
the sample text. If you have a condensed version of the font open, you can
change the width of the sample text. This allow you to see how the different
faces of the font family work together. You can clean all formatting from the
selected text by clicking on the "Clear Formatting" button.

An additional tool menu button, also located in the text area toolbar, allows
to import and export the sample text in a JSON format, which preserves the
formatting of the text. This can be useful if you are working on a specific
feature, like diacritics, and want to test it with different fonts or font
families again and again.

As with the :doc:`metrics view </ui/mainviews/metricsview>` you may set the
script and language of the sample text and then choose which features to apply.
Note that the first option of "Automatic shaping" guesses the language of each
paragraph and shapes the text automatically according to it. Setting explicit
script and language overrides this automatic behavior and allows to test more
specific OpenType features.

.. _printing.full_font_display:

Full Font Display
^^^^^^^^^^^^^^^^^

All the characters of your font, in a rectangular grid:

  .. image:: /images/FullFont.png

.. _printing.glyph_display:

Glyph Display
^^^^^^^^^^^^^

Certain selected characters, one per page, at a scale to fill up the page. This
mode supports three scaling options:

* *Scale each glyph to page size* - each glyph is scaled to fill the page,
  regardless of its original size.

* *Scale glyphs to em size* - all the glyphs are scaled uniformly so that the
  em height is scaled to the page height. In this mode some glyphs might protrude
  outside the page height or the page width.

* *Scale glyphs to maximum height* - all the glyphs are scaled uniformly to
  accomodate the highest ascenders and the lowest descenders within the page
  height. In this mode some glyphs might still be wider than the page width.

  .. image:: /images/FullPage.png

.. _printing.multisize_glyph_display:

Multisize Glyph Display
^^^^^^^^^^^^^^^^^^^^^^^

Certain selected characters at various pointsizes (72, 48, 36, 24, 20, 18, 16,
15, 14, 13, 12, 11, 10, 9, 8, 7.5, 7, 6.5, 6, 5.5, 5, 4.5, 4)

  .. image:: /images/MultisizeGlyph.png

-------------------------------------------------------------------------------

Obviously the samples are based on european and CJK character sets. If anyone
has a small section of text in any language not represented please send a copy
to the developers. Copyright free, of course, and preferably in a unicode encoding...

:doc:`Sources of the samples </appendices/quotations>`...