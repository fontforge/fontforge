Major differences between FontForge's and Adobe's interpretation of feature files
=================================================================================

Not really any, any more. Except that FontForge supports much of the syntax
which adobe documents but does not support. Adobe continues to reserve the right
to change the syntax of anything they do not currently support.

In November 2008 Adobe made radical changes to the feature file format. This
meant that my extensions were no longer needed. It also as an incompatible
change from the earlier format. FontForge should (still) be able to parse files
written in the earlier format, as well as files written in the new format. It
will only produce files in the new format now (actually there is a compile time
flag which will revert output to the old format, but I haven't tested it).


Parsing a feature file
----------------------

Use :ref:`File->Merge Feature Info <filemenu.Merge-feature>` (formerly
:menuselection:`File --> Merge Kern Info`)


Outputting a feature file
-------------------------

Use :ref:`Element->Font Info->Lookups <fontinfo.Lookups>`, then right click on
any lookup (to produce a popup menu) and select ``Save Feature File``

You can also generate a feature file containing a single lookup by selecting
that lookup, producing the popup menu and selecting ``Save Lookup``