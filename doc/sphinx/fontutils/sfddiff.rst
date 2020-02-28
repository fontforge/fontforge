sfddiff
=======

A program for comparing fonts ::

   sfddiff [-merge outfile] [-ignorehints] [-ignorenames] [-ignoregpos] [-ignoregsub] [-ignorebitmaps] [-exact] [-warn] [-version] [-help] fontfile1 fontfile2

*Sfddiff* compares (or merges) two font files (in any format
:doc:`fontforge </index>` can read) and checks for differences.

It will notice:

#. any characters present in one font but not in the other
#. any characters present in both fonts but which have different sets of
   outlines or references. The outlines may be compared so that only exact
   matches are accepted, or so that a fuzzier match is used (useful in compare
   that a truetype and postscript font are the same). Similarly references may
   be matched exactly or may match after an unlink.
#. optionally will check if the postscript hints or truetype instructions are
   different.
#. if the truetype 'name' tables match
#. if kerning (and other 'GPOS' data) match
#. if ligatures (and other 'GSUB' data) match
#. Any bitmap strikes present in one font but not the other
#. Any bitmap characters present in one font but not the other
#. Any bitmap characters which differ

.. program:: sfddiff

.. option:: -merge <output>

   If this flag is specified the following argument should be the name of an
   output file into which sfddiff will store a merged version of the two fonts.

   This will contain everything from fontfile1 as well as any characters present in
   fontfile2 but not in fontfile1. For any characters with different outlines or
   references, the background of the character will contain the splines from
   fontfile2 (sadly references can not be placed in the background).

.. option:: -ignorehints

   If specified, then no hint differences will be reported.

.. option:: -help

   Provides a mini description and will list the available options.

.. option:: -usage

   Lists the available options.

.. option:: -version

   Displays the current version of *sfddiff*.


See Also
--------

:doc:`fontforge </index>`