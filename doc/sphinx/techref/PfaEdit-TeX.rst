.. _PfaEdit-TeX.TeX:

FontForge and TeX
=================

FontForge has a number of features built in to it to deal with TeX.

It can read "pk" and "gf" bitmap files and
:ref:`autotrace <elementmenu.AutoTrace>` them to generate outline fonts. It can
even invoke metafont directly on a ".mf" file, generate a "gf" bitmap from that,
autotrace it and generate an outline font.

It can read ligature, kerning information from a ".tfm" file. It can create a
tfm file.

The `encodings <https://fontforge.org/downloads/Encodings.ps.gz>`__ file has a number of standard TeX
encodings built in to it.

For large CJK truetype fonts it can read a sub-font definition file as defined
in the ttf2tfm man page and :doc:`generate </ui/dialogs/generate>` a series of postscript
type-1 fonts based on those two.

In consultation with the lilypond group we have designed a new
:doc:`SFNT table 'TeX <non-standard>` ' for storing TeX specific information in
True/OpenType files.


Generating a tfm file (and an enc file).
----------------------------------------

Before you generate a tfm file you should perform some of the following

.. object:: Set the font parameters

   Do this with :menuselection:`Element --> Font Info --> TeX`.

.. _PfaEdit-TeX.Italic:

.. object:: Optionally set Italic Correction values for glyphs

   (FontForge will generate default values for italic fonts, so you only need do
   this if FontForge gets it wrong).

   * Select a glyph which should have an Italic Correction
   * :menuselection:`Element --> Glyph Info --> Alt Pos`
   * Press [New]
   * Set the tag to ``Italic Correction`` from the pulldown list
   * Set the XAdvance to the italic correction

.. _PfaEdit-TeX.charlist:

.. object:: Optionally set any glyphlists

   * Select the first glyph in the glyphlist (the smallest one)
   * :menuselection:`Element --> Glyph Info --> Alt Subs`
   * Press [New]
   * Set the tag to ``TeX Glyphlist`` from the pulldown list
   * Type the names of all the other glyphs in the glyphlist into the Components
     field

     So if the charlist is for left parenthesis

     You would select the lparen glyph and might type in "lparen.big lparen.bigger
     lparen.biggest"

.. _PfaEdit-TeX.extension:

.. object:: Optionally set any extension glyphs.

   * If the glyph is in a charlist then Select the last glyph in the glyphlist
     (the largest one)
   * :menuselection:`Element --> Glyph Info --> Mult Subs`
   * Press [New]
   * Set the tag to ``TeX Extension list`` from the pulldown list
   * Type the names of the glyphs that this glyph decomposes into in the order:
     top middle bottom extension

     If a field is not present use .notdef

     So if the extension list is for left parenthesis

     You would select the lparen.biggest glyph and might type in "lparen.top
     lparen.mid lparen.bottom lparen.ext"

Note: When FontForge does a :menuselection:`File --> Merge Feature Info` on a
tfm file, it will set these values appropriately.

Having done this to your satisfaction you are ready to generate a tfm file. Go
to :menuselection:`File --> Generate Fonts`, select one of the postscript
encodings from the pulldown list, press the [Options] button and turn on the [*]
Tfm & Enc check box.

I'm not sure what you *do* with these files yet, but this should create them.


.. _PfaEdit-TeX.TeX-Install:

Installing a type1 (pfb) postscript font for TeX
------------------------------------------------

I am a novice TeX/LaTeX user so my comments should be taken with a grain of
salt. I did manage to get this process to work on my own system.

Installing a PostScript font for TeX is more complicated than one would hope
(and so far I've only figured out how to install a Latin font). Instead of just
moving the font file to some standard directory you must:

* <configure TeX so that it is prepared for local additions>
* Rename the font file so that the filename is in a format TeX understands

  (I'm told this isn't required, but I couldn't get things to work without doing
  this. Perhaps I have an old system. Perhaps I didn't try hard enough)
* Create several helper files that TeX will use for its own purposes
* Move each file type into its own special directory
* Optionally: Create a package file to make it easy for LaTeX to find the font
* Optionally: Move the package file into its own directory
* Use the updmap script or manually:

  * Update dvips's configuration files so that it knows where to look for the
    postscript fonts
  * Optionally: Update pdftex's configuration files so that it knows where to look
    too.

I suggest that before you read further you look at the following resources on
the web:

* It is possible to add your fonts to the standard TeX directory structure, but
  the TeX guru's frown on this as it makes updating TeX difficult. They suggest
  instead that you make all your changes in some parallel directory and provide
  they instructions on how to go about doing this at:
  `Installation advice for TeX fonts and directories <http://www.ctan.org/installationadvice/>`_.
  They also provide an example of a font installation but that is better explained
  in
  `the LaTeX font-faq <http://www.ctan.org/tex-archive/info/Type1fonts/fontinstallationguide.pdf>`_.
* Old versions of TeX (ie. mine) are still worried about the old 8 character
  limitation on DOS filenames. This means that the tools I was using don't accept
  understandable filenames instead they require a format described in
  `TeX font file naming conventions <http://www.tug.org/fontname/html/index.html>`_.
  If you are creating your own fonts this boils down to:

  * the first letter of the font should be "f" (which means the font wasn't made by
    one of the big-name font vendors)
  * the next two letters are some abbreviation of the family-name for your font
  * the next letter (or two) should be "r" for a roman font, "i" for italic, "o" for
    oblique, "b" for bold, and "bi" for bold italic
  * the last two letters should be "8a" (which means your font is in Adobe Standard
    Encoding. And your font *must be* in that encoding or things don't work).

    (Again I am told that you can use any encoding as long as you register it with
    TeX. I was unable to get this to work. But my system is out of date)
* Finally
  `the LaTeX font-faq <http://www.ctan.org/tex-archive/info/Type1fonts/fontinstallationguide.pdf>`_
  describes the gory details of how to go about installing the font. Its only
  (minor) drawback is that it assumes you are installing a font from Adobe. That
  is easily glossed over it means that:

  * fonts from Adobe should have font names beginning with "p" rather than "f".
  * the translation from adobe's font-families to 2 character abbreviations has
    already been done, when using an adobe font you look the family up in a table to
    get the 2 character abbreviation, when creating your own font you make up your
    own.
  * the vendor directory for adobe is "adobe", but the vendor directory for fonts
    you make should be "public"
  * (I hope these comments will make sense after you've read the above links)
* I've not tried to work with truetype fonts, but here's a document that talks
  about it to some extent.
  `LaTeX and TTF <http://www.radamir.com/tex/ttf-tex.htm>`_
* If you are interested in the basics, here's the documentation on
  `fontinst <http://www.ctan.org/tex-archive/fonts/utilities/fontinst/doc/fontinst.ps>`_
  itself.
* I don't know how to deal with cyrillic (except that cyrillic T2 encodings are
  called 6a), greek or CJK fonts yet.
* Here's some info on how to use fonts once they are installed:
  `LaTeX and fonts <http://www-h.eng.cam.ac.uk/help/tpl/textprocessing/fonts.html>`_

I did the following:

* I created a directory structure as described in
  `Installation advice <http://www.ctan.org/installationadvice/>`_ (and also in
  `the LaTeX font-faq <http://www.ctan.org/tex-archive/info/Type1fonts/fontinstallationguide.pdf>`_).
* I made a font (which I will call Cupola), initially I encoded it with the TeX
  Base Encoding (this was to make sure I had all the characters I needed)
* Then just before generating it I reencoded it into Adobe Standard Encoding
  (because TeX's fontinst routine expects that)
* I generated the font naming it "fcur8a.pfb" which means:

  * f -- made by a small font vendor, public domain, etc.
  * cu -- abbreviation for the family name "Cupola"
  * r -- roman face
  * 8a -- Adobe Standard Encoding
* I applied the following script:

  .. code-block:: bash

     #!/bin/bash
     # You will need to change the next two lines to suit your font.
     # You may need to change the two after that as well.
     BASE=fcu
     PACKAGE=cupola
     VENDOR=public
     LOCALTEXMF=/usr/local/share/texmf
     
     # remove any old files that might be lying around and might confuse us later on
     csh -c "rm fi.tex *.mtx *.pl *.vpl"
     
     # create a little script to get TeX to create various useful files it needs
     echo "\\input fontinst.sty" >fi.tex
     echo "\\latinfamily{$BASE}{}" >>fi.tex
     echo "\\bye" >>fi.tex
     
     #execute that script
     tex fi
     
     # But we need to do a bit more processing on some of those files
     for file in *.pl ; do
     pltotf $file
     done
     for file in *.vpl ; do
     vptovf $file
     done
     
     # Get rid of stuff we don't need any more
     rm fi.tex *.mtx *.pl *.vpl
     
     # create the directories we need for the various components
     mkdir -p $LOCALTEXMF/fonts/type1/$VENDOR/$PACKAGE \
             $LOCALTEXMF/fonts/afm/$VENDOR/$PACKAGE \
             $LOCALTEXMF/fonts/tfm/$VENDOR/$PACKAGE \
             $LOCALTEXMF/fonts/vf/$VENDOR/$PACKAGE \
             $LOCALTEXMF/tex/latex/$VENDOR/$PACKAGE
     
     # move everything into its expected directory
     cp $BASE*.pfb $LOCALTEXMF/fonts/type1/$VENDOR/$PACKAGE
     cp $BASE*.afm $LOCALTEXMF/fonts/afm/$VENDOR/$PACKAGE
     mv $BASE*.tfm $LOCALTEXMF/fonts/tfm/$VENDOR/$PACKAGE
     mv $BASE*.vf $LOCALTEXMF/fonts/vf/$VENDOR/$PACKAGE
     mv *$BASE*.fd $LOCALTEXMF/tex/latex/$VENDOR/$PACKAGE
     
     # finally create the LaTeX package for this font (and put it in the right place)
     echo "\\ProvidesPackage{$PACKAGE}" > $LOCALTEXMF/tex/latex/$VENDOR/$PACKAGE/$PACKAGE.sty
     echo "\\renewcommand{\\rmdefault}{$BASE}" >> $LOCALTEXMF/tex/latex/$VENDOR/$PACKAGE/$PACKAGE.sty
     echo "\\endinput" >> $LOCALTEXMF/tex/latex/$VENDOR/$PACKAGE/$PACKAGE.sty
     
     # but updating the map files required a bit more knowlege than this script has
     # so I left that to be done by hand
     echo "*********************************************************************"
     echo You need to create your own map files
     echo One should be called $LOCALTEXMF/dvips/config/$BASE.map and should
     echo " contain a line for each file in the family. One might look like this:"
     echo "${BASE}r8a $PACKAGE-Regular \"TexBase1Encoding ReEncodeFont\" <8r.enc <${BASE}r8a.pfb"
     echo Then change the config.ps file by looking for the location defining the
     echo " standard map file and adding:"
     echo "p +$BASE.map"
     echo " after it."
     echo Then go to $LOCALTEXMF/pdftex/config/
     echo Make a copy "(or a link)" of $LOCALTEXMF/dvips/config/$BASE.map
     echo and edit pdftex.cfg and insert
     echo "p +$BASE.map"
     echo at the appropriate place in it too.
