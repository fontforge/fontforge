Palm fonts
==========

Fonts on the palm pilot are rather limited. Each strike is stored in its own
record of a database with no name or indication of whether it is connected to
any other font (ie. there is no concept of a family of fonts. Each strike is
alone).

On old palm pilots the font data structure stored a strike for a single screen
resolution. On newer pilots there is a different data structure which can store
multiple strikes for several screen resolutions (called "density" on the palm).

The new style font is unusable (causes crashes) on old palms. I am told that the
old style font causes crashes on new systems.

I'm told that at the moment new systems are all the same (high) density (so
there is currently no need for a multi-density font -- what is needed is a
single (high) density font in a multi-density wrapper).

I am told that to be portable an application currently needs to supply an old
style low-res font for old style palms, and a new-style font containing just a
high-res strike.

The palm OS reference documents the following densities:

* single
* one and a half
* double
* triple
* quadruple

I'm told that only single and double are actually used. FontForge does not
support the 1.5 density font.

If you want to generate an old-style (low-res) palm font you should specify a
single strike size in the Generate Font bitmap sizes field.

If you specify two or more strike sizes in the bitmap sizes field then all sizes
must be a small integer multiple (1,2,3,4) of the smallest strike. A dlg will
come up asking whether you want these strikes:

* All within a multi-density font
* All (except the smallest) within a multi-density font -- FontForge calls this a
  high density font
* An old style low-res font containing the smallest strike in the first record of
  the database file and a new style multi density font containing all strikes
  within the second record of the generated file.
* An old style low-res font containing the smallest strike in the first record of
  the database file and a new style multi density font containing all strikes
  *except the smallest* within the second record of the generated file.

I believe the last format to be the most useful. This is the default, and is the
format used for the script Generate command.

Note: You must include the low-density strike. This strike defines the metrics
for all the others. However you may generate a font which does not include this
strike (the second choice in the list above).