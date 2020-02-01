Cidmap files
============

These are FontForge's own files which it uses to map from CIDs in one
Registry-Ordering-Supplement to unicode code points.

Cidmap files generally live in ``$(PREFIX)/share/fontforge`` (often
``/usr/local/share/fontforge`` or ``/usr/share/fontforge``), though they will
also be found if they are in the directory containing fontforge itself or the
current directory. They are named ``<registry>-<Ordering>-<Supplement>.cidmap``.
So ``"Adobe-Japan1-6.cidmap"``, or ``"Adobe-CNS1-4.cidmap"``.

The first line of the file consists of two numbers, usually the same, which are
the total number of CIDs defined by this ROS.

Subsequent lines have one of three formats

* <cid> <unicode>
* <cid1>..<cid2> <unicode>
* <cid> <postscript-name>

A <cid> is just a decimal number, <unicode> is a hex number, and
<postscript-name> is a PostScript name token preceded by a slash.

The first few lines of Adobe-Japan1-6.cidmap look like:

::

   23057 23057
   0 /.notdef
   1..60 0020
   61 00a5
   62..92 005d
   93 00a6

So there are 23057 cids in this ROS. CID 0 is mapped to the postscript name
"/.notdef", cids 1-60 are mapped to space (U+0020), exclam (U+0021), ...,
lbracket (U+005b), CID 61 is mapped to yen (U+00a5), and so forth.