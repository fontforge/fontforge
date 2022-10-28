Scripting FontForge
===================

FontForge includes two interpreters so you can write scripts to modify fonts.
One of these interpreters is :doc:`python </scripting/python>`, one is a legacy language I
came up with. FontForge may be configured with either or both of these. If
configured with both then fontforge will make a guess at which to use based on
the script file's extension ("py" means use the python interpreter, "ff" or "pe"
means use the old interpreter)

.. toctree::

   scripting-alpha
   python

.. contents::
   :backlinks: none

.. default-domain:: ff

.. _scripting.Starting:

Invoking scripts
----------------

If you start fontforge with a script on the command line it will not put up any
windows and it will exit when the script is done. The script can be in a file,
or just a string presented as an argument. You may need to specify which
interpreter to use with the -lang argument.

::

   $ fontforge -script scriptfile.pe {arguments}
   $ fontforge -c "script-string" {arguments}
   $ fontforge -lang={ff|py} -c "script-string"

FontForge can also be used as an interpreter to which the shell will
automatically pass scripts. If you a mark your script files as executable ::

   $ chmod +x scriptfile.pe

and begin each one with the line ::

   #!/usr/local/bin/fontforge

(or wherever fontforge happens to reside on your system) then you can invoke the
script just by typing ::

   $ scriptfile.pe {fontnames}

If you wish FontForge to read a script from stdin then you can use "-" as a
"filename" for stdin. (If you build FontForge without X11 then fontforge will
attempt to read a script file from ``stdin`` if none is given on the command
line.)

You can also start a script from within FontForge with
:menuselection:`File --> Execute Script`, and you can use the Preference Dlg to
define a set of frequently used scripts which can be invoked directly by menu.

The scripting language provides access to much of the functionality found in the
font view's menus. It does not currently (and probably never will) provide
access to everything. (If you find a lack let me know, I may put it in for you).
It does not provide commands for building up a glyph out of splines, instead it
allows you to do high level modifications to glyphs.

If you set the environment variable ``FONTFORGE_VERBOSE`` (it doesn't need a
value, just needs to be set) then FontForge will print scripts to stdout as it
executes them.

You may set the environment variable ``FONTFORGE_LANGUAGE`` to either "py" (for
python) or "ff" or "pe" (for native scripting) as another way to determne what
interpreter to use.


.. _scripting.Language:

Scripting Language
------------------

.. note::

   This section covers FontForge's native scripting functionality.

   For the Python scripting functionality, see :doc:`here </scripting/python>`.

The syntax is rather like a mixture of C and shell commands. Every file
corresponds to a procedure. As in a shell script arguments passed to the file
are identified as $1, $2, ... $n. $0 is the file name itself. $argc gives the
number of arguments. $argv[<expr>] provides array access to the arguments.

Terms can be

* A variable name (like "$1" or "i" or "@fontvar" or "_global")

  The scope of the variable depends on the initial character of its name.

  * A '$' signifies that it is a built-in variable. The user cannot create any new
    variables beginning with '$'. Some, but not all, of these may be assigned to.
  * A '_' signifies that the variable is global, it is always available. You can use
    these to store context across different script files (or to access data within
    nested script files).
  * A '@' signifies that the variable is associated with the font. Any two scripts
    looking at the same font will have access to the same variables.
  * A variable which begins with a letter is a local variable. It is only meaningful
    within the current script file. Nested script files may have different variables
    with the same names.
* an integer expressed in decimal, hex or octal
* a unicode code point (which has a prefix of "0u" or "0U" and is followed by a
  string of hex digits. This is only used by the select command.
* a real number (in "C" locale format -- "." as the decimal point character)
* A string which may be enclosed in either double or single quotes. String tokens
  are limited to 256 bytes. "\n" can be used to represent a newline character. (If
  you need longer strings use the concatenation operator).
* a procedure to call or file to invoke.
* an expression within parentheses
* a series of expressions (separated by commas) within brackets, as
  ``[1, 2, 3, 5, 8]``, used to create an array.

There are three different comments supported:

* Starting with a "#" character and proceeding to end of line
* Starting with "//" and proceeding to end of line
* Starting with "/*" and proceeding to "*/"

.. _scripting.Expressions:

Expressions
^^^^^^^^^^^

Expressions are similar to those in C, a few operators have been omitted, a few
added from shell scripts. Operator precedence has been simplified slightly. So
operators (and their precedences) are:

* unary operators (+, -, !, ~, ++ (prefix and postfix), --(prefix and postfix), ()
  (procedure call), [] (array index), :h, :t, :r, :e

  Most of these are as expected in C, the last four are borrowed from shell
  scripts and are applied to strings

  * :h gives the head (directory) of a pathspec
  * :t gives the tail (filename) of a pathspec
  * :r gives the pathspec without the extension (if any)
  * :e gives the extension
* \*, /, % (binary multiplicative operators)
* +, - (binary arithmetic operators)

  If the first operand of + is a string then + will be treated as concatenation
  rather than addition. If the second operand is a number it will be converted to
  a string (decimal representation) and then concatenated. If the first operand of
  + is an array then + will do array concatenation -- if the second argument is
  also an array the two will be concatenated ([1,2] + [3,4] yields [1,2,3,4],
  while [1,2] + 3 yields [1,2,3]). Otherwise these are the normal arithmetric
  operations.
* ==, !=, >, <, >=, <= (comparison operators, may be applied to either two
  integers or two strings)
* &&, & (logical and, bitwise and. (logical and will do short circuit evaluation))
* \|\|, \|, ^ (logical or, bitwise or, bitwise exclusive or (logical or will do short
  circuit evaluation))
* =, +=, -=, \*=, /=, %= (assignment operators as in C. The += will act as
  concatenation if the first operand is a string.)

Note there is no comma operator, and no "?:" operator. The precedence of "and"
and "or" has been simplified, as has that of the assignment operators.

Procedure calls may be applied either to a name token, or to a string. If the
name or string is recognized as one of FontForge's internal procedures it will
be executed, otherwise it will be assumed to be a filename containing another
fontforge script file, this file will be invoked (since filenames can contain
characters not legal in name tokens it is important to allow general strings to
specify filenames). If the procedure name does not contain a directory then it
is assumed to be in the same directory as the current script file. As a
convenience, it is not necessary to include the script's extension if it is
either ".ff" or ".pe". At most 25 arguments can be passed to a procedure.

Arrays are passed by reference, strings and integers are passed by value.

Variables may be created by assigning a value to them (only with the "="), so: ::

   i=3

could be used to define "i" as a variable. Variables are limited in scope to the
current file, they will not be inherited by called procedures.

A statement may be

* an expression
* ::

       if ( expression )
           statements
       {elseif ( expression )
           statements}
       [else
           statements]
       endif
* ::

       while ( expression )
           statements
       endloop
* ::

       foreach
           statements
       endloop

* ``break``
* ``return [ expression ]``
* ``shift``

As with C, non-zero expressions are defined to be true.

A return statement may be followed by a return value (the expression) or a
procedure may return nothing (void).

The shift statement is stolen from shell scripts and shifts all arguments down
by one. (argument 0, the name of the script file, remains unchanged.

The foreach statement requires that there be a current font. It executes the
statements once for each glyph in the selection. Within the statements only one
glyph at a time will be selected. After execution the selection will be restored
to what it was initially. (Caveat: Don't reencode the font within a foreach
statement).

Statements are terminated either by a new line (you can break up long lines with
backslash newline) or a semicolon.

Trivial example:

::

   i=0;   #semicolon is not needed here, but it's ok
   while ( i<3 )
      if ( i==1 /* pointless comment */ )
           Print( "Got to one" )   // Another comment
      endif
      ++i
   endloop

FontForge maintains the concept of a "current font"-- almost all commands refer
only to the current font (and require that there be a font). If you start a
script with :menuselection:`File --> Execute Script`, the font you were editing
will be current, otherwise there will be no initial current font. The Open(),
New() and Close() commands all change the current font. FontForge also maintains
a list of all fonts that are currently open. This list is in no particular
order. The list starts with $firstfont.

Similarly when working with cid keyed fonts, FontForge works in the "current sub
font", and most commands refer to this font. The CIDChangeSubFont() command can
alter that.

.. _scripting.variables:

Variables
^^^^^^^^^

All builtin variables begin with "$", you may not create any variables that
start with "$" yourself (though you may assign to (some) already existing ones)

.. data:: $0

   the current script filename

.. data:: $1
          $2
          $n

   the first, second (...) argument to the script file

.. data:: $argc

   the number of arguments passed to the script file (this will always be
   at least 1 as $0 is always present)

.. data:: $argv

   allows you to access the array of all the arguments

.. data:: $curfont

   the name of the filename in which the current font resides

.. data:: $firstfont

   the name of the filename of the font which is first on the font
   list (Can be used by :func:`Open()`), if there are no fonts loaded this
   returns an empty string. This can be used to determine if any font at all
   is loaded into fontforge.

.. data:: $nextfont

   the name of the filename of the font which follows the current
   font on the list (or the empty string if the current font is the last one on the
   list)

.. data:: $fontchanged

   returns 1 if the current font has changed, 0 if it has not
   changed since it was read in (or saved).

.. data:: $fontname

   the name contained in the postscript FontName field

.. data:: $familyname

   the name contained in the postscript FamilyName field

.. data:: $fullname

   the name contained in the postscript FullName field

.. data:: $fondname

   if set this name indicates what FOND the current font should be
   put in under Generate Mac Family.

.. data:: $weight

   the name contained in the postscript Weight field

.. data:: $copyright

   the name contained in the postscript Notice field

.. data:: $filename

   the name of the file containing the font.

.. data:: $fontversion

   the string containing the font's version

.. data:: $iscid

   1 if the current font is a cid keyed font, 0 if not

.. data:: $cidfontname

   returns the fontname of the top-level cid-keyed font (or the
   empty string if there is none)

   Can be used to detect if this is a cid keyed font.

.. data:: $cidfamilyname, $cidfullname, $cidweight, $cidcopyright

   similar to :data:`cidfontname`

.. data:: $mmcount

   returns 0 for non multiple master fonts, returns the number of
   instances in a multiple master font.

.. data:: $italicangle

   the value of the postscript italic angle field

.. data:: $loadState

   a bitmask of non-fatal errors encountered when loading the font.

.. data:: $privateState

   a bitmask of some errors in the PostScript Private dictionary
   (see :py:attr:`fontforge.font.privateState` for more info).

.. data:: $curcid

   returns the fontname of the current font

.. data:: $firstcid

   returns the fontname of the first font within this cid font

.. data:: $nextcid

   returns the fontname of the next font within this cid font (or the
   empty string if the current sub-font is the last)

.. data:: $macstyle

   returns the value of the macstyle field (a set of bits indicating
   whether the font is bold, italic, condensed, etc.)

.. data:: $bitmaps

   returns an array containing all bitmap pixelsizes generated for
   this font. (If the font database contains greymaps then they will be indicated
   in the array as ``(<BitmapDepth><<16)|<PixelSize>``)

.. data:: $order

   returns an integer containing either 2 (for truetype fonts) or 3 (for
   postscript fonts). This indicates whether the font uses quadratic or cubic
   splines.

.. data:: $em

   returns the number of em-units used by the font.

.. data:: $ascent

   returns the ascent of the font

.. data:: $descent

   returns the descent of the font.

.. data:: $selection

   returns an array containing one entry for each glyph in the
   current font indicating whether that glyph is selected or not (0=>not,
   1=>selected)

.. data:: $panose

   returns an array containing the 10 panose values for the font.

.. data:: $trace

   if this is set to one then FontForge will trace each procedure call.

.. data:: $version

   returns a string containing the current version of fontforge. This
    should look something like "20050817".

.. data:: $haspython

   returns 1 if python scripting is available, 0 if it is not.


.. _scripting.Preference:

Preferences
^^^^^^^^^^^

* ``$<Preference Item>`` (for example ``$AutoHint``) allows you to examine the
  value of that preference item (to set it use
  :func:`SetPref()`)

The following example will perform an action on all loaded fonts:

::

   file = $firstfont
   while ( file != "" )
      Open(file)
      /* Do Stuff */
      file = $nextfont
   endloop

.. _scripting.procedures:

The built in procedures are very similar to the menu items with the same names.
Often the description here is sketchy, look at the menu item for more
information.

* :doc:`Built-in procedures in alphabetic order <scripting-alpha>`
* :ref:`Built-in procedures that do not require a font <scripting.no-font>`
* :ref:`File menu <scripting.file-menu>`
* :ref:`File manipulation <scripting.files>`
* :ref:`Edit menu <scripting.edit-menu>`
* :ref:`Select menu <scripting.select-menu>`
* :ref:`Element menu <scripting.element-menu>`
* :ref:`Font information <scripting.font-info>`
* :ref:`Glyph information <scripting.glyph-info>`
* :ref:`Advanced Typography <scripting.ATT>`
* :ref:`Encoding menu <scripting.encoding-menu>`
* :ref:`Hint menu <scripting.hint-menu>`
* :ref:`Metrics menu <scripting.metrics-menu>`
* :ref:`Multiple master <scripting.MM>`
* :ref:`CID keyed fonts <scripting.CID>`
* :ref:`User interaction <scripting.user>`
* :ref:`Preferences <scripting.prefs>`
* :ref:`Math <scripting.math>`
* :ref:`Unicode <scripting.unicode>`
* :ref:`String manipulation <scripting.strings>`
* :ref:`Character manipulation <scripting.Character>`
* :ref:`Arrays <scripting.arrays>`
* :ref:`Miscellaneous <scripting.misc>`
* :ref:`Deprecated Names <scripting.Deprecated>`


.. _scripting.alpha:

Built-in procedures in alphabetic order
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _scripting.no-font:

Built-in procedures that do not require a loaded font
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`Array()`
* :func:`AskUser()`
* :func:`ATan2()`
* :func:`Ceil()`
* :func:`Chr()`
* :func:`Cos()`
* :func:`Floor()`
* :func:`Error()`
* :func:`Exp()`
* :func:`DefaultOtherSubrs()`
* :func:`FileAccess()`
* :func:`FontsInFile()`
* :func:`GetEnv()`
* :func:`GetPref()`
* :func:`Int()`
* :func:`IsAlNum()`
* :func:`IsAlpha()`
* :func:`IsDigit()`
* :func:`IsFinite()`
* :func:`IsHexDigit()`
* :func:`IsLower()`
* :func:`IsNan()`
* :func:`IsSpace()`
* :func:`IsUpper()`
* :func:`LoadEncodingFile()`
* :func:`LoadNamelist()`
* :func:`LoadNamelistDir()`
* :func:`LoadPrefs()`
* :func:`LoadStringFromFile()`
* :func:`Log()`
* :func:`NameFromUnicode()`
* :func:`New()`
* :func:`Open()`
* :func:`Ord()`
* :func:`PostNotice()`
* :func:`Pow()`
* :func:`PreloadCidmap()`
* :func:`Print()`
* :func:`Rand()`
* :func:`ReadOtherSubrsFile()`
* :func:`Real()`
* :func:`Round()`
* :func:`SavePrefs()`
* :func:`SetPref()`
* :func:`SizeOf()`
* :func:`Sin()`
* :func:`Sqrt()`
* :func:`Strcasestr()`
* :func:`Strcasecmp()`
* :func:`Strftime()`
* :func:`StrJoin()`
* :func:`Strlen()`
* :func:`Strrstr()`
* :func:`Strskipint()`
* :func:`StrSplit()`
* :func:`Strstr()`
* :func:`Strsub()`
* :func:`Strtod()`
* :func:`Strtol()`
* :func:`Tan()`
* :func:`ToLower()`
* :func:`ToMirror()`
* :func:`ToString()`
* :func:`ToUpper()`
* :func:`TypeOf()`
* :func:`UCodePoint()`
* :func:`UnicodeFromName()`
* :func:`Ucs4()`
* :func:`Utf8()`
* :func:`WriteStringToFile()`


.. _scripting.file-menu:

Built-in procedures that act like the File Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`Close()`
* :func:`Export()`
* :func:`FontsInFile()`
* :func:`Generate()`
* :func:`GenerateFamily()`
* :func:`Import()`
* :func:`MergeKern()` deprecated
* :func:`MergeFeature()`
* :func:`New()`
* :func:`Open()`
* :func:`PrintFont()`
* :func:`PrintSetup()`
* :func:`Quit()`
* :func:`Revert()`
* :func:`RevertToBackup()`
* :func:`Save()`


.. _scripting.files:

File Manipulation
^^^^^^^^^^^^^^^^^

* :func:`FileAccess()`
* :func:`FontImage()`
* :func:`LoadStringFromFile()`
* :func:`WriteStringToFile()`


.. _scripting.edit-menu:

Built-in procedures that act like the Edit Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`Clear()`
* :func:`ClearBackground()`
* :func:`Copy()`
* :func:`CopyAnchors()`
* :func:`CopyFgToBg()`
* :func:`CopyLBearing()`
* :func:`CopyRBearing()`
* :func:`CopyReference()`
* :func:`CopyUnlinked()`
* :func:`CopyVWidth()`
* :func:`CopyWidth()`
* :func:`Cut()`
* :func:`Join()`
* :func:`Paste()`
* :func:`Paste()`
* :func:`PasteWithOffset()`
* :func:`ReplaceWithReference()`
* :func:`SameGlyphAs()`
* :func:`UnlinkReference()`


.. _scripting.select-menu:

Built-in procedures that act like the Select Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`Select()`
* :func:`SelectAll()`
* :func:`SelectAllInstancesOf()`
* :func:`SelectBitmap()`
* :func:`SelectByPosSub()`
* :func:`SelectChanged()`
* :func:`SelectFewer()`
* :func:`SelectFewerSingletons()`
* :func:`SelectGlyphsBoth()`
* :func:`SelectGlyphsReferences()`
* :func:`SelectGlyphsSplines()`
* :func:`SelectHintingNeeded()`
* :func:`SelectIf()`
* :func:`SelectInvert()`
* :func:`SelectMore()`
* :func:`SelectMoreIf()`
* :func:`SelectMoreSingletons()`
* :func:`SelectMoreSingletonsIf()`
* :func:`SelectNone()`
* :func:`SelectSingletons()`
* :func:`SelectSingletonsIf()`
* :func:`SelectWorthOutputting()`


.. _scripting.element-menu:

Built-in procedures that act like the Element Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`AddAccent()`
* :func:`AddExtrema()`
* :func:`AddInflections()`
* :func:`ApplySubstitution()`
* :func:`AutoTrace()`
* :func:`Balance()`
* :func:`BitmapsAvail()`
* :func:`BitmapsRegen()`
* :func:`BuildAccented()`
* :func:`BuildComposite()`
* :func:`BuildDuplicate()`
* :func:`CanonicalContours()`
* :func:`CanonicalStart()`
* :func:`ChangeWeight()`
* :func:`CompareFonts()`
* :func:`CompareGlyphs()`
* :func:`CorrectDirection()`
* :func:`DefaultRoundToGrid()`
* :func:`DefaultUseMyMetrics()`
* :func:`ExpandStroke()`
* :func:`FindIntersections()`
* :func:`Harmonize()`
* :func:`HFlip()`
* :func:`Inline()`
* :func:`InterpolateFonts()`
* :func:`Italic()`
* :func:`MergeFonts()`
* :func:`Move()`
* :func:`MoveReference()`
* :func:`NearlyHvCps()`
* :func:`NearlyHvLines()`
* :func:`NearlyLines()`
* :func:`NonLinearTransform()`
* :func:`Outline()`
* :func:`OverlapIntersect()`
* :func:`PositionReference()`
* :func:`RemoveOverlap()`
* :func:`Rotate()`
* :func:`RoundToCluster()`
* :func:`RoundToInt()`
* :func:`Scale()`
* :func:`ScaleToEm()`
* :func:`Shadow()`
* :func:`Simplify()`
* :func:`Skew()`
* :func:`SmallCaps()`
* :func:`Transform()`
* :func:`VFlip()`
* :func:`Wireframe()`


.. _scripting.font-info:

Font Info
^^^^^^^^^

* :func:`AddSizeFeature()`
* :func:`ChangePrivateEntry()`
* :func:`ClearPrivateEntry()`
* :func:`GetFontBoundingBox()`
* :func:`GetMaxpValue()`
* :func:`GetOS2Value()`
* :func:`GetPrivateEntry()`
* :func:`GetTeXParam()`
* :func:`GetTTFName()`
* :func:`HasPrivateEntry()`
* :func:`ScaleToEm()`
* :func:`SetFondName()`
* :func:`SetFontHasVerticalMetrics()`
* :func:`SetFontNames()`
* :func:`SetFontOrder()`
* :func:`SetGasp()`
* :func:`SetItalicAngle()`
* :func:`SetMacStyle()`
* :func:`SetMaxpValue()`
* :func:`SetOS2Value()`
* :func:`SetPanose()`
* :func:`SetTeXParams()`
* :func:`SetTTFName()`
* :func:`SetUniqueID()`
* Some items (such as the font name) may be retrieved via built-in variables.


.. _scripting.glyph-info:

Glyph Info
^^^^^^^^^^

* :func:`DrawsSomething()`
* :func:`GetPosSub()`
* :func:`GlyphInfo()`
* :func:`SetGlyphColor()`
* :func:`SetGlyphComment()`
* :func:`SetGlyphChanged()`
* :func:`SetGlyphClass()`
* :func:`SetGlyphName()`
* :func:`SetUnicodeValue()`
* :func:`SetGlyphTeX()`
* :func:`WorthOutputting()`


.. _scripting.ATT:

Built-in procedures that handle Advanced Typography
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`AddAnchorClass()`
* :func:`AddAnchorPoint()`
* :func:`AddLookup()`
* :func:`AddLookupSubtable()`
* :func:`AddPosSub()`
* :func:`AddSizeFeature()`
* :func:`ApplySubstitution()`
* :func:`CheckForAnchorClass()`
* :func:`GetAnchorPoints()`
* :func:`GetLookupInfo()`
* :func:`GetLookups()`
* :func:`GetLookupSubtables()`
* :func:`GetLookupOfSubtable()`
* :func:`GetPosSub()`
* :func:`GetSubtableOfAnchor()`
* :func:`GenerateFeatureFile()`
* :func:`HasPreservedTable()`
* :func:`LoadTableFromFile()`
* :func:`LookupStoreLigatureInAfm()`
* :func:`LookupSetFeatureList()`
* :func:`MergeLookups()`
* :func:`MergeLookupSubtables()`
* :func:`RemoveAnchorClass()`
* :func:`RemoveLookup()`
* :func:`RemoveLookupSubtable()`
* :func:`RemovePosSub()`
* :func:`RemovePreservedTable()`
* :func:`SaveTableToFile()`


.. _scripting.encoding-menu:

Built-in procedures that act like the Encoding Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`CharCnt()`
* :func:`DetachGlyphs()`
* :func:`DetachAndRemoveGlyphs()`
* :func:`LoadEncodingFile()`
* :func:`MultipleEncodingsToReferences()`
* :func:`Reencode()`
* :func:`RemoveDetachedGlyphs()`
* :func:`RenameGlyphs()`
* :func:`SameGlyphAs()`
* :func:`SetCharCnt()`


.. _scripting.hint-menu:

Built-in procedures that act like the Hint Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`AddDHint()`
* :func:`AddHHint()`
* :func:`AddInstrs()`
* :func:`AddVHint()`
* :func:`AutoCounter()`
* :func:`AutoHint()`
* :func:`AutoInstr()`
* :func:`ChangePrivateEntry()`
* :func:`ClearGlyphCounterMasks()`
* :func:`ClearHints()`
* :func:`ClearInstrs()`
* :func:`ClearPrivateEntry()`
* :func:`ClearTable()`
* :func:`DontAutoHint()`
* :func:`FindOrAddCvtIndex()`
* :func:`GetCvtAt()`
* :func:`GetPrivateEntry()`
* :func:`HasPrivateEntry()`
* :func:`ReplaceGlyphCounterMasks()`
* :func:`ReplaceCvtAt()`
* :func:`SetGlyphCounterMask()`
* :func:`SubstitutionPoints()`


.. _scripting.metrics-menu:

Built-in procedures that act like the Metrics Menu
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`AutoKern()`
* :func:`AutoWidth()`
* :func:`CenterInWidth()`
* :func:`SetKern()`
* :func:`RemoveAllKerns()`
* :func:`RemoveAllVKerns()`
* :func:`SetLBearing()`
* :func:`SetRBearing()`
* :func:`SetVKern()`
* :func:`SetVWidth()`
* :func:`SetWidth()`
* :func:`VKernFromHKern()`


.. _scripting.MM:

Multiple master routines
^^^^^^^^^^^^^^^^^^^^^^^^

* :func:`MMAxisBounds()`
* :func:`MMAxisNames()`
* :func:`MMBlendToNewFont()`
* :func:`MMChangeInstance()`
* :func:`MMChangeWeight()`
* :func:`MMInstanceNames()`
* :func:`MMWeightedName()`


.. _scripting.CID:

CID routines
^^^^^^^^^^^^

* :func:`CIDChangeSubFont()`
* :func:`CIDFlatten()`
* :func:`CIDFlattenByCMap()`
* :func:`CIDSetFontNames()`
* :func:`ConvertToCID()`
* :func:`ConvertByCMap()`
* :func:`PreloadCidmap()`


.. _scripting.user:

User Interaction
^^^^^^^^^^^^^^^^

* :func:`AskUser()`
* :func:`Error()`
* :func:`PostNotice()`
* :func:`Print()`


.. _scripting.prefs:

Preferences
^^^^^^^^^^^

* :func:`DefaultOtherSubrs()`
* :func:`GetPref()`
* :func:`LoadEncodingFile()`
* :func:`LoadNamelist()`
* :func:`LoadNamelistDir()`
* :func:`LoadPrefs()`
* :func:`ReadOtherSubrsFile()`
* :func:`SavePrefs()`
* :func:`SetPref()`
* It it also possible to get the value of a preference item by preceding its name
  with a dollar sign.


.. _scripting.math:

Math
^^^^

* :func:`ATan2()`
* :func:`Ceil()`
* :func:`Chr()`
* :func:`Cos()`
* :func:`Exp()`
* :func:`Floor()`
* :func:`Int()`
* :func:`IsFinite()`
* :func:`IsNan()`
* :func:`Log()`
* :func:`Ord()`
* :func:`Pow()`
* :func:`Rand()`
* :func:`Real()`
* :func:`Round()`
* :func:`Sin()`
* :func:`Sqrt()`
* :func:`Strskipint()`
* :func:`Strtod()`
* :func:`Strtol()`
* :func:`Tan()`
* :func:`ToString()`
* :func:`UCodePoint()`


.. _scripting.unicode:

Unicode
^^^^^^^

* :func:`NameFromUnicode()`
* :func:`UCodePoint()`
* :func:`UnicodeFromName()`
* :func:`Ucs4()`
* :func:`Utf8()`


.. _scripting.strings:

String manipulation
^^^^^^^^^^^^^^^^^^^

* :func:`Chr()`
* :func:`GetEnv()`
* :func:`NameFromUnicode()`
* :func:`Ord()`
* :func:`Strcasecmp()`
* :func:`Strcasestr()`
* :func:`Strftime()`
* :func:`StrJoin()`
* :func:`Strlen()`
* :func:`Strrstr()`
* :func:`Strskipint()`
* :func:`StrSplit()`
* :func:`Strstr()`
* :func:`Strsub()`
* :func:`Strtod()`
* :func:`Strtol()`
* :func:`ToString()`
* :func:`UnicodeFromName()`
* :func:`Ucs4()`
* :func:`Utf8()`


.. _scripting.Character:

Character Manipulation
^^^^^^^^^^^^^^^^^^^^^^

* :func:`IsAlNum()`
* :func:`IsAlpha()`
* :func:`IsDigit()`
* :func:`IsHexDigit()`
* :func:`IsLower()`
* :func:`IsSpace()`
* :func:`IsUpper()`
* :func:`ToLower()`
* :func:`ToMirror()`
* :func:`ToUpper()`


.. _scripting.arrays:

Arrays
^^^^^^

* :func:`Array()`
* :func:`SizeOf()`


.. _scripting.misc:

Miscellaneous
^^^^^^^^^^^^^

* :func:`InFont()`
* :func:`TypeOf()`


.. _scripting.Deprecated:

Deprecated Names
^^^^^^^^^^^^^^^^

* :func:`ClearGlyphCounterMasks()`
* :func:`GlyphInfo()`
* :func:`ReplaceGlyphCounterMasks()`
* :func:`SetGlyphColor()`
* :func:`SetGlyphComment()`
* :func:`SetGlyphCounterMask()`
* :func:`SetGlyphName()`

--------------------------------------------------------------------------------


.. _scripting.Example:

Examples
--------

* `FontForge's testsuite in the test subdirectory <https://github.com/fontforge/fontforge/tree/master/tests>`__
  (such as it is)
* `Directory of donated scripts <https://github.com/fontforge/fontforge/tree/master/pycontrib>`__
* Scripts used in other projects

  * `x-symbol <http://sourceforge.net/projects/x-symbol/>`__
* :doc:`the scripting tutorial </tutorial/scripting-tutorial>`


Example 1:
^^^^^^^^^^

::

   #Set the color of all selected glyphs to be yellow
   #designed to be run within an interactive fontforge session.
   foreach
     SetCharColor(0xffff00)
   endloop


Example 2:
^^^^^^^^^^

::

   #!/usr/local/bin/fontforge
   #This is the sfddiff script which compares two fonts
   
   if ( Strtol($version) < 20060330 )
     Error( "Please upgrade to a more recent version of fontforge" )
   endif
   
   flags=0x789
   outfile=""
   
   while ( $argc > 1 && Strsub($1,0,1)=="-" )
     temp = $1
     if ( Strsub(temp,1,2)=='-' )
       temp = Strsub(temp,1)
     endif
   
     if ( temp=="-ignorehints" )
       flags = flags & ~0x8
     elseif ( temp=="-ignorenames" )
       flags = flags & ~0x100
     elseif ( temp=="-ignoregpos" )
       flags = flags & ~0x200
     elseif ( temp=="-ignoregsub" )
       flags = flags & ~0x400
     elseif ( temp=="-ignorebitmaps" )
       flags = flags & ~0x80
     elseif ( temp=="-exact" )
       flags = flags | 0x2
     elseif ( temp=="-warn" )
       flags = flags | 0x44
     elseif ( temp=="-merge" )
       flags = flags | 0x1800
       shift
       outfile = $1
     elseif ( temp=="-help" )
       Print( "sfddiff: [--version] [--help] [--usage] [--ignorehints] [--ignorenames] [--ignoregpos] [--ignoregsup] [--ignorebitmaps] [--warn] [--exact] fontfile1 fontfile2" )
       Print( " Compares two fontfiles" )
       Print( " --ignorehints: Do not compare postscript hints or truetype instructions" )
       Print( " --ignorenames: Do not compare font names" )
       Print( " --ignoregpos:  Do not compare kerning, etc." )
       Print( " --ignoregsub:  Do not compare ligatures, etc." )
       Print( " --ignorebitmaps: Do not compare bitmap strikes" )
       Print( " --exact:       Normally sfddiff will match contours which are not exact" )
       Print( "                but where the differences are slight (so you could compare" )
       Print( "                truetype and postscript and get reasonable answers). Also" )
       Print( "                normally sfddiff will unlink references before it compares" )
       Print( "                (so you can compare a postscript font (with no references)" )
       Print( "                to the original source (which does have references)). Setting")
       Print( "                this flag means glyphs must match exactly.")
       Print( " --warn:        Provides a warning when an exact match is not found" )
       Print( " --merge outfile: Put any outline differences in the backgrounds of" )
       Print( "                appropriate glyphs" )
   return(0)
     elseif ( temp=="-version" )
       Print( "Version 1.0" )
   return(0)
     else
   break
     endif
     shift
   endloop
   
   if ( $argc!=3 || $1=="--usage" || $1=="-usage" )
     Print( "sfddiff: [--version] [--help] [--usage] [--ignorehints] [--ignorenames] [--ignoregpos] [--ignoregsup] [--ignorebitmaps] [--warn] [--exact] [--merge outfile] fontfile1 fontfile2" )
   return(0)
   endif
   
   Open($2)
   Open($1)
   CompareFonts($2,"-",flags)
   if ( outfile!="" )
     Save(outfile)
   endif


Example 3:
^^^^^^^^^^

::

   #!/usr/local/bin/fontforge
   #Take a Latin font and apply some simple transformations to it
   #prior to adding cyrillic letters.
   #can be run in a non-interactive fontforge session.
   Open($1);
   Reencode("KOI8-R");
   Select(0xa0,0xff);
   //Copy those things which look just like latin
   BuildComposit();
   BuildAccented();
   
   //Handle Ya which looks like a backwards "R"
   Select("R");
   Copy();
   Select("afii10049");
   Paste();
   HFlip();
   CorrectDirection();
   Copy();
   Select(0u044f);
   Paste();
   CopyFgToBg();
   Clear();
   
   //Gamma looks like an upside-down L
   Select("L");
   Copy();
   Select(0u0413);
   Paste();
   VFlip();
   CorrectDirection();
   Copy();
   Select(0u0433);
   Paste();
   CopyFgToBg();
   Clear();
   
   //Prepare for editing small caps K, etc.
   Select("K");
   Copy();
   Select(0u043a);
   Paste();
   CopyFgToBg();
   Clear();
   
   Select("H");
   Copy();
   Select(0u043d);
   Paste();
   CopyFgToBg();
   Clear();
   
   Select("T");
   Copy();
   Select(0u0442);
   Paste();
   CopyFgToBg();
   Clear();
   
   Select("B");
   Copy();
   Select(0u0432);
   Paste();
   CopyFgToBg();
   Clear();
   
   Select("M");
   Copy();
   Select(0u043C);
   Paste();
   CopyFgToBg();
   Clear();
   
   Save($1:r+"-koi8-r.sfd");
   Quit(0);


.. _scripting.Execute:

The Execute Script dialog
-------------------------

This dialog allows you to type a script directly in to FontForge and then run
it. Of course the most common case is that you'll have a script file somewhere
that you want to execute, so there's a button [Call] down at the bottom of the
dlg. Pressing [Call] will bring up a file picker dlg looking for files with the
extension \*.pe (you can change that by typing a wildcard sequence and pressing
the [Filter] button). After you have selected your script the appropriate text
to text to invoke it will be placed in the text area.

The current font of the script will be set to whatever font you invoked it from
(in python this is :py:func:`fontforge.activeFont()`).

Note that if you try to print from a script the output will go to stdout. If you
have invoked fontforge from a window manager's menu stdout will often be bound
to /dev/null and not useful. Try starting fontforge from the command line
instead.


.. _scripting.menu:

The Scripts Menu
----------------

You can use the preference dialog to create a list of frequently used scripts.
Invoke :ref:`File->Preferences <prefs.scripts>` and select the Scripts tag. In
this dialog are ten possible entries, each one should have a name (to be
displayed in the menu) and an associated script file to be run.

After you have set up your preferences you can invoke scripts from the font
view, either directly from the menu
(:menuselection:`File --> Scripts --> <your name>`) or by a hot key. The first
script you added will be invoked by Cnt-Alt-1, then second by Cnt-Alt-2, and the
tenth by Cnt-Alt-0.

The current font of the script will be set to whatever font you invoked it from.
