fontforge
=========

.. default-domain:: py

.. module:: fontforge
   :synopsis: The primary module for interacting with FontForge

Module attributes
-----------------

.. data:: hooks

   A dictionary which the user may fill to associate certain FontForge
   events with a python function to be run when those events happen.
   The function will be passed the font (or possibly glyph) for which
   the relevant event occurred.

   **Hook names**

   .. object:: newFontHook

      This function will be called when a new font has been created.

   .. object:: loadFontHook

      This function will be called when a font is loaded from disk.
      (if a font has an "initScriptString" entry in its persistent
      dictionary, that script will be invoked before this function).

   Other hooks are defined in a font's own temporary and persistent dictionaries.

.. data:: splineCorner

   A point type enumeration of value 0

.. data:: splineCurve

   A point type enumeration of value 1

.. data:: splineHVCurve

   A point type enumeration of value 2

.. data:: splineTangent

   A point type enumeration of value 3

.. data:: fontforge.spiroG4

   A spiro point type enumeration of value 1.

   A Spiro G4 curve point

.. data:: fontforge.spiroG2

   A spiro point type enumeration of value 2.

   A Spiro G2 curve point

.. data:: fontforge.spiroCorner

   A spiro point type enumeration of value 3.

   A Spiro corner point

.. data:: fontforge.spiroLeft

   A spiro point type enumeration of value 4.

   A Spiro left "tangent" point

.. data:: fontforge.spiroRight

   A spiro point type enumeration of value 5.

   A Spiro right "tangent" point

.. data:: fontforge.spiroOpen

   A spiro point type enumeration of value 6.

   This may only be used on the first point in a spiro tuple. It indicates that the tuple
   describes an open contour.

.. data:: fontforge.unspecifiedMathValue

   A constant, used when the value is unspecified

Module functions
----------------

.. function:: getPrefs(pref_name)

   returns the value of the named preference item

.. function:: setPrefs(pref_name, value)

   sets the value of the named preference item

.. function:: hasSpiro()

   Returns a boolean, ``True`` if Raph Levien's spiro package is available for
   use in FontForge.

.. function:: savePrefs()

   Saves the current preference settings

.. function:: loadPrefs()

   Loads the user's default preference settings. Not done automatically in a
   script.

.. function:: defaultOtherSubrs()

   Sets the type1 PostScript OtherSubrs to the default value

.. function:: readOtherSubrsFile(filename)

   Sets the type1 PostScript OtherSubrs to the stuff found in the file.

.. function:: loadEncodingFile(filename[, encname])

   Loads an encoding file, returns the name of the encoding or ``None``. When
   loading encodings in Unicode consortium format, an encname has to be specefied
   or the encoding will be ignored and ``None`` will be returned.

.. function:: loadNamelist(filename)

   Loads a namelist

.. function:: loadNamelistDir(dirname)

   Loads all namelist files in the directory

.. function:: preloadCidmap(filename, registry, order, supplement)

   Loads a FontForge cidmap file (first three args are strings, last is an integer)

.. function:: printSetup(type[, printer|cmd|file, width, height])

   Prepare to :func:`print a font sample <font.printSample>`.
   The first argument may be one of:

   .. object:: lp

      Queues postscript output to the printer using lp.
      You may use the optional second argument to specify the printer name.

   .. object:: lpr

      Queues postscript output to the printer using lpr.
      You may use the optional second argument to specify the printer name.

   .. object:: ghostview

      Displays the output using ghostview (or gv). The second argument is ignored.

   .. object:: command

      Use a custom shell command to print the output.
      The second argument should contain the command and its arguments.

   .. object:: ps-file

      Dump the postscript output to a file. The second argument specifies the filename.

   .. pdf-file

      Dump the output as pdf to a file. The second argument specifies the filename.

   The third and fourth arguments are optional and specify the page size
   (in points) for the output. The third argument is the page width and the
   fourth is the height. These settings remain until changed.

.. function:: nameFromUnicode(uni[, namelist])

   Finds the glyph name associated with a given unicode codepoint. If a
   namelist is specified the name will be taken from that.

.. function:: UnicodeAnnotationFromLib(n)

   Returns the Unicode Annotations for this value as described by
   www.unicode.org. If there is no unicode annotation for this value, or no
   library available, then return empty string "". It can execute with no
   current font.

.. function:: UnicodeBlockCountFromLib(n)

   Return the number of Unicode Blocks as described by www.unicode.org.
   Currently, the blocks are {0..233}, spanning unicode values {uni0..uni10FFFF}.
   If there is no value, or no library available, then return -1. This can
   execute with no current font.

.. function:: UnicodeBlockEndFromLib(n)

   Returns the Unicode Block end value as described by www.unicode.org.
   Currently, the blocks are {0..233}, spanning unicode values {uni0..uni10FFFF}.
   If there is no value, or no library available, then return -1. This can
   execute with no current font.

.. function:: UnicodeBlockNameFromLib(n)

   Returns the Unicode Block Name as described by www.unicode.org.
   Currently, the blocks are {0..233}, spanning unicode values {uni0..uni10FFFF}.
   If there is no value, or no library available, then return empty string "".
   This can execute with no current font.

.. function:: UnicodeBlockStartFromLib(n)

   Returns the Unicode Block start value as described by www.unicode.org.
   Currently, the blocks are {0..233}, spanning unicode values {uni0..uni10FFFF}.
   If there is no value, or no library available, then return -1.
   This can execute with no current font.

.. function:: unicodeFromName(glyphname)

   Looks up glyph name in its dictionary and if it is associated with a unicode
   code point returns that number. Otherwise it returns -1

.. function:: UnicodeNameFromLib(n)

   Returns the Unicode Name for this value as described by www.unicode.org.
   If there is no unicode name for this value, or no library available, then
   return empty string "". It can execute with no current font.

.. function:: UnicodeNamesListVersion()

   Return the Unicode Nameslist Version (as described by www.unicode.org).
   libuninameslist is released on a schedule that depends on when www.unicode.org
   releases new information. These dates do not match FontForge release dates,
   therefore users might not keep this optional library upto current updates.
   This instruction can be used to test if the Nameslist library is recent for
   your script. This function currently works only for libuninameslist
   ver_0.3.20130501 or later, else it returns empty string "". This can execute
   with no current font.

.. function:: UnicodeNames2GetCntFromLib()

   Return the Total Count of all Names that were corrected with a new name.
   Errors and corrections happen, therefore names can be corrected in the next
   Unicode Nameslist version. If there is no libuninameslist ver 0.5 or later
   available, then return -1.

.. function:: UnicodeNames2GetNxtFromLib(n)

   Errors and corrections happen, therefore names can be corrected in the next
   Unicode Nameslist version. With val==unicode value, this function returns -1
   if no Names2 exists, or the Nth table location for this unicode value listed
   in libuninameslist that was corrected to a new name. If there is no
   libuninameslist ver 0.5 or later, then return -1.

.. function:: UnicodeNames2NxtUniFromLib(n)

   Errors and corrections happen, therefore names can be corrected in the next
   Unicode Nameslist version. This function returns the Next Names2 listed in
   libuninameslist internal table that was corrected to a new name. The internal
   table of Unicode values is of size :func:`UnicodeNames2GetCntFromLib()`.
   If there is no libuninameslist ver 0.5 or later, then return -1.

.. function:: UnicodeNames2FrmTabFromLib(n)

   Errors and corrections happen, therefore names can be corrected in the next
   Unicode Nameslist version. This function returns the Next Names2 listed in
   libuninameslist internal table that was corrected to a new name. The internal
   table of Unicode values is of size :func:`UnicodeNames2GetCntFromLib()`. If
   there is no libuninameslist ver 0.5 or later, then return ``None``.

.. function:: UnicodeNames2FromLib(val)

   Errors and corrections happen, therefore names can be corrected in the next
   Unicode Nameslist version. This function returns the Names2 or ``None`` based
   on the unicode value given. If there is no libuninameslist ver 0.5 or later,
   then return ``None``.

.. function:: scriptFromUnicode(n)

   Return the script tag for the given Unicode codepoint. So, for ``ord('Q')``,
   it would return ``latn``. This is most useful with :meth:`font.addLookup()`,
   like: ::

      # Add a `mark` lookup for an arbitrary glyph...
      script = fontforge.scriptFromUnicode(glyph.unicode)
      font.addLookup("l1", "gpos_mark2base", None, (("mark",((script,("dflt")),)),))
      font.addLookupSubtable("l1", "l1-1")
      font.addAnchorClass("l1-1", "top")

.. function:: IsFraction(n)

   Return 1 if n is a unicode fraction (either a vulgar fraction or other
   fraction) as described by www.unicode.org. Return 0 if there is no fraction
   for this value. It can execute with no current font.

.. function:: IsLigature(n)

   Return 1 if n is a ligature as described by www.unicode.org. Return 0 if there
   is no unicode ligature for this value. It can execute with no current font.

.. function:: IsOtherFraction(n)

   Return 1 if n is a unicode fraction (not defined as vulgar fraction) as
   described by www.unicode.org. Return 0 if there is no fraction for this
   value. It can execute with no current font.

.. function:: IsVulgarFraction(n)

   Return 1 if n is a unicode vulgar fraction as described by www.unicode.org.
   Return 0 if there is no fraction for this value. It can execute with no
   current font.

.. function:: ucFracChartGetCnt()

   Returns total count of Fractions found in the Unicode chart as described by
   www.unicode.org. It can execute with no current font. Note: Count depends on
   chart version built into FontForge.

.. function:: ucLigChartGetCnt()

   Returns total count of Ligatures found in the Unicode chart as described by
   www.unicode.org. It can execute with no current font. Note: Count depends on
   chart version built into FontForge.

.. function:: ucLigChartGetLoc(val)

   Returns n for FontForge internal table Unicode ``val=Ligature[n]``. If val
   does not exist in table, then return -1. Can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucLigChartGetNxt(n)

   Returns FontForge internal table Unicode Ligature[n]. Return -1 if ``n<0``
   or ``n>=ucLigChartGetCnt()``. It can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucOFracChartGetCnt()

   Returns total count of non-Vulgar Fractions found in the Unicode chart as
   described by www.unicode.org. It can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucOFracChartGetLoc(val)

   Returns n for FontForge internal table Unicode ``val=OtherFraction[n]``. If
   val does not exist in table, then return -1. Can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucOFracChartGetNxt(n)

   Returns FontForge internal table Unicode (non-vulgar) Fraction[n]. Return -1
   if ``n<0`` or ``n>=ucOFracChartGetCnt()``. Can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucVulChartGetCnt()

   Returns total count of Vulgar Fractions found in the Unicode chart as
   described by www.unicode.org. It can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucVulChartGetLoc(val)

   Returns n for FontForge internal table Unicode ``val=VulgarFraction[n]``. If
   val does not exist in table, then return -1. Can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: ucVulChartGetNxt(n)

   Returns FontForge internal table Unicode Vulgar Fraction[n]. Returns -1 if
   n<0 or ``n>=ucVulChartGetCnt()``. Can execute with no current font.

   .. note::

      Count depends on chart version built into FontForge.

.. function:: SpiroVersion()

   Returns the version of LibSpiro available to FontForge. Versions 0.1 to 0.5
   do not have a method to indicate version numbers, but there is a limited
   method to estimate versions {'0.0'..'0.5'}. '0.0' if FontForge has no LibSpiro
   available. '0.1' if LibSpiro 20071029 is available. '0.2' if LibSpiro 0.2 to
   0.5 is available. LibSpiro 0.6 and higher reports back it's version available.

.. function:: version()

   Returns FontForge's version number. This will be a large number like 20070406.

.. function:: runInitScripts()

   Runs the system or user initialization scripts, if not already run. This is
   primarily intended when importing FontForge into a python process.

.. function:: scriptPath()

   Returns a tuple listing the directory paths which are searched for python
   scripts during FontForge initialization.

.. function:: fonts()

   Returns a tuple of all fonts currently loaded into FontForge for editing

.. function:: activeFont()

   If the script were invoked from the File->Execute Script... dialog, or
   invoked by a menu item in the font view, this returns the font that was
   active at the time. Otherwise it returns ``None``.

.. function:: activeGlyph()

   If the script were invoked from the File->Execute Script... dialog or a menu
   item from an outline glyph window or a glyph import/export command this
   returns the glyph that was active at the time. Otherwise it returns ``None``.

.. function:: activeLayer()

   This returns the currently active layer as an integer between 0 (inclusive)
   and the font/glyph's layer count (exclusive). It may also be set to -1 if the
   current glyph window is displaying the font's guideline layer.

.. function:: fontsInFile(filename)

   Returns a tuple of all font names found in the specified file. The tuple may
   be empty if FontForge couldn't find any.

.. function:: open(filename[, flags])

   Opens a filename and returns the font it contains (if any). The optional
   ``flags`` argument can be string tuple or integer combination of the
   following flags:

   .. object:: fstypepermitted (1)

      The user has the appropriate license to examine the font no matter what
      the fstype setting is.

   .. object:: allglyphsinttc (4)

      Load all glyphs from the 'glyf' table of a ttc font (rather than only the
      glyphs used in the font picked).

   .. object:: fontlint (8)

      Report more error conditions.

   .. object:: hidewindow (16)

      Do not create a view window for this font even if the UI is active.

      .. note::

         This option supports efficient bulk processing of fonts in scripts run
         through the UI but using it can be tricky. Open fonts will be listed at
         the bottom of the "Window" menu but choosing them will have no effect.

         If some fonts are not closed you may need to "force-quit" the
         application using your OS.

   .. object:: alltables (32)

      Retain all recognized font tables that do not have a native format.

.. function:: parseTTInstrs(string)

   Returns a binary string each byte of which corresponds to a truetype
   instruction. The input string should contain a set of instruction names as ::

      "SRP0 MIRP[min,rnd,black]"

.. function:: unParseTTInstrs(sequence)

   Reverse of the above. Converts a binary string into a human (sort of)
   readable string

.. function:: unitShape(n)

   Returns a closed contour which is a regular n-gon. The contour will be
   inscribed in the unit circle. If n is negative, then the contour will be
   circumscribed around the unit circle. A value of 0 will produce a unit circle.
   If n==1 it is treated as if n were -4 -- a circumscribed square where each
   side is 2 units long (this is for historical reasons). Behavior is undefined
   for n=2,-1,-2.

.. function:: registerGlyphSeparationHook(hook)

   The GlyphSeparationHook is a python routine which FontForge will call when
   it wants to figure out the optical separation between two glyphs. If you
   never call this, or if you call it with a value of ``None`` FontForge will
   use a built-in default. This routine gets called during AutoWidth, AutoKern,
   and computing the optical left and right side bearings (for 'lfbd' and 'rtbd'
   features). For more information see its own section.


.. _fontforge.ui_functions:

User Interface Module Functions
-------------------------------

Users may define scripts to be run when menu items are invoked. Some of these
scripts will want to ask users questions, so this section provides routines to
determine if FontForge has a user interface, a command to add menu items, and
various small standard dialogs to interact with the user. I do not currently
provide a mechanism for allowing people to define special purpose dialogs (for
example they might want to ask more than one question in a dialog, and I don't
support that).

When FontForge starts (if it's a FontForge with python) it will look at the
directories ``$(PREFIX)/share/fontforge/python`` and ``~/.FontForge/python``
and attempt to run all files in those directories which end in ``".py"``.
Presumably these files will allow people to customize the user interface to
suit their needs.

Currently it reads the files in directory order (which is generally somewhere
between creation order and totally random). It will read the system directory
before the user directory.

.. rubric:: Example

::

   import fontforge

   def nameGlyph(junk, glyph):
      print(glyph.glyphname)

   fontforge.registerMenuItem(nameGlyph, None, None, "Glyph", None, "Print Glyph Name")

   def neverEnableMe(junk, glyph):
      return False

   fontforge.registerMenuItem(nameGlyph, neverEnableMe, None, "Glyph", None, "SubMenu", "Print Glyph Name")

   def importGlyph(junk, glyph, filename, toback):
      print("Import")
      print(glyph.glyphname)
      print(filename)

   def exportGlyph(junk, glyph, filename):
      print("Import")
      print(glyph.glyphname)
      print(filename)

   fontforge.registerImportExport(importGlyph, exportGlyph, None, "foosball", "foo", "foo,foobar")

The first call will define a menu item in the Tools menu of the Glyph window.
The menu will be called "Print Glyph Name". It has no shortcut to invoke it. It
needs no external data. It is always enabled. And when activated it will invoke
the function "nameGlyph" which prints the name of the glyph in the window from
which the command is invoked.

The second call defines a menu item in a submenu of the Tools menu. This submenu
is called "SubMenu". This item will never be enabled -- but if it were enabled
it would again call "nameGlyph" to print the name of the current glyph.

The last provides a way to import and export files of type "foosball" (or it
would if the routines did anything).

Not a very useful example.


.. function:: hasUserInterface()

   Returns ``True`` if this session of FontForge has a user interface

.. function:: registerMenuItem(menu_function, enable_function, data, which_window, shortcut_string, {submenu_names, } menu_name_string)

   If FontForge has a user interface this will add this menu item to FontForge's :ref:`Tool <toolsmenu.tools>`
   menu, either in the font or the outline glyph view (or both).

   .. object:: menu-function

      This is the function that will be called when the menu item is activated.
      It will be passed two arguments, the first is the data value specified here
      (which may be ``None``, indeed will probably usually be ``None``), and the
      second is the glyph or font (depending on the window type) from which the
      menu item was activated. Its return value is ignored.

   .. object:: enable_function

      This may be ``None`` -- in which case the menu item will always be enabled.
      Otherwise it will be called before the menu pops up on the screen to
      determine whether this item should be enabled. It will be passed the same
      arguments as above. It should return ``True`` if the item should be
      enabled and ``False`` otherwise.

   .. object:: data

      This can be whatever you want (including ``None``). FontForge keeps track
      of it and passes it to both of the above functions. Use it if you need to
      provide some context for the menu item.

   .. object:: which_window

      May be either of the strings ``"Font"`` or ``"Glyph"`` (or the tuple
      ``("Font", "Glyph")``) and it determines which type of window will have
      this menu item in its "Tools" menu.

   .. object:: shortcut-string

      .. warning:: Deprecated?

      May be ``None`` if you do not wish to supply a shortcut. Otherwise should
      be a string like "Menu Name|Cntl-H" (the syntax is defined in the
      translation section).

   .. object:: submenu-names

      You may specify as many of these as you wish (including leaving them out
      altogether), this allows you to organize the Tools menu into submenus. (If
      a submenu of this name does not currently exist, FontForge will create it).

   .. object:: menu-name

      The name that will appear in the menu for this item. This will only affect
      windows created after this command is executed. Normally the command will
      be executed at startup and so it will affect all windows.

.. function:: registerImportExport(import_function, export_function, data, name, extension, [extension_list])

   This will add the capability to import or export files of a given type,
   presumably a way of specifying the splines in a given glyph.

   .. object:: import-function

      The function to call to import a file into a glyph. It will be called
      with: The data argument (specified below), A pointer to the glyph into
      which the import is to happen, A filename, A flag indicating whether the
      import should go to the background layer or foreground. This function may
      be ``None``. In which case there is no import method for this file type.

   .. object:: export-function

      The function to call to export a glyph into a file. It will be called
      with: The data argument (see below), a pointer to the glyph, and a
      filename. This function may be ``None``, in which case there is no export
      method for this file type.

   .. object:: data

      Anything you like (including ``None``). It will be passed to the
      import/export routines and can provide them with context if they need that.
      name The name to be displayed in the user interface for this file type.
      This may just be the extension, or it might be something more informative.

   .. object:: extension

      This is the default extension for this file type. It is used by the
      export dialog to pick an extension for the generated filename.

   .. object:: extension-list

      Some file types have more than one common extension (eps files are usually
      named "eps", but I have also seen "ps" and "art" used). The import dialog
      needs to filter all possible filenames of this file type. This argument
      should be a comma separated list of extensions. It may be omitted, in
      which case it defaults to being the same as the "extension" argument above.

.. function:: logWarning(msg)

   Adds the message (a string) to FontForge's Warnings window. (if you wish to
   display a % character you must represent it as two percents). If there is no
   user interface the output will go to stderr.

.. function:: postError(win_title, msg)

   Creates a popup dialog to display the message (a string) in that dlg. (if you
   wish to display a % character you must represent it as two percents). If
   there is no user interface the output will go to stderr.

.. function:: postNotice(win_title, msg)

   Creates a little window which will silently vanish after a minute or two and
   displays the message (a string) in that window. (if you wish to display a %
   character you must represent it as two percents). If there is no user
   interface the output will go to stderr.

.. function:: openFilename(question, [def_name, [filter]])

   All arguments are strings. The first is a question asked to the user (for
   which a filename to open is presumed to be the answer). The second is
   optional and provides a default filename. The third is optional and provides
   a filter (like "\*.sfd")

   The result is either a filename or ``None`` if the user canceled the dialog.

   Throws an exception if there is no user interface.

.. function:: saveFilename(question, [def_name, [filter]])

   All arguments are strings. The first is a question asked to the user (for
   which a filename to save something to is presumed to be the answer). The
   second is optional and provides a default filename. The third is optional and
   provides a filter (like "\*.sfd")

   The result is either a filename or ``None`` if the user canceled the dialog.

   Throws an exception if there is no user interface.

.. function:: ask(title, question, answers, [def, cancel])

   Allows you to ask the user a multiple choice question. It pops up a dialog
   posing the question with a list of buttons ranged underneath it -- one for
   each answer.

   The first argument is the dialog's title, the second is the question to be
   asked, the third is a tuple of strings -- each string will become a button,
   the fourth and fifth arguments are option, the fourth is the index in the
   answer array that will be the default answer (the one invoked if the user
   presses the [Return] key), and the fifth is the answer invoked if the user
   presses the [Escape] key. If omitted the default answer will be the first,
   and the cancel answer will be the last.

   The function returns the index in the answer array of the answer chosen by
   the user.

   Throws an exception if there is no user interface.

.. function:: askChoices(title, question, answers[,default=,multiple=])

   Similar to the above allows you to ask the user a multiple choice question.
   It pops up a dialog posing the question with a scrollable list of choices --
   one for each answer.

   The first argument is the dialog's title, the second is the question to be
   asked, the third is a tuple of strings -- each string will become a button,
   the fourth and fifth arguments are option, the fourth is the index in the
   answer array that will be the default answer (the one invoked if the user
   presses the [Return] key). If omitted the default answer will be the first.

   The fifth argument means that multiple options can be selected. If true,
   the fourth argument should be a tuple of Boolean values or a single integer
   index into the answer tuple. So, if there are three options, it should look
   like ``(True, False, True)``, which would select the first and last option.

   The function returns the index in the answer array of the answer chosen by
   the user. If the user
   cancels the dialog, a -1 is returned.

   ``default`` and ``multiple`` may be passed by position if desired.

   Throws an exception if there is no user interface.

.. function:: askString(title, question, [def_string])

   Allows you to as the user a question for which a string is the answer.

   The first argument is the dialog's title, the second is the question to be
   asked, the third is optional and specified a default answer.

   The function returns the string the user typed or ``None`` if they cancelled
   the dialog.

   Throws an exception if there is no user interface.



Point
-----

.. class:: point([x, y, on_curve, type, selected])

   Creates a new point. Optionally specifying its x,y location,
   on-curve status and selected status. x and y must be supplied together.

   A "point initializer tuple" is any tuple (x,y[,True|False[,0|1|2|3[,True|False]]]),
   where x and y are numbers, the third value corresponds to on-curve, the
   fourth to type, and the fifth to selected.

.. attribute:: point.x

   The x location of the point

.. attribute:: point.y

   The y location of the point

.. attribute:: point.on_curve

   Whether this is an on curve point or an off curve point (a control point)

.. attribute:: point.selected

   The value of this member also determines the selected status in the UI on
   setting a layer. FontForge usually retains the selection status of any point
   between and that of an on-curve point when saving, whether or not a UI is present.

.. attribute:: point.type

   For an on-curve point, its FontForge point type.

   There are four types: :data:`fontforge.splineCorner`, :data:`fontforge.splineCurve`,
   :data:`fontforge.splineHVCurve` and :data:`fontforge.splineTangent`.

   A new point will have type :data:`splineCorner`. When assigning a layer to
   :attr:`glyph.layers`, :attr:`glyph.background` or :attr:`glyph.foreground`
   the type value is ignored. To influence the type FontForge will associate
   with an on-curve point use :meth:`glyph.setLayer`.

.. attribute:: point.interpolated

   ``True`` if FontForge treats this (quadratic, on-curve) point as interpolated.
   All interpolated points should be mid-way between their off-curve points,
   but some such points are not treated as interpolated. This flag is ignored
   when setting a layer.

   Older versions of FontForge omitted interpolated points. This was equivalent
   to executing the following on a contour: ::

      c[:] = [ p for p in c if not p.interpolated ]

   This member will be false for a point marked "Never interpolate" in FontForge
   but there is currently no way of setting or preserving that mark when a layer
   is replaced using the Python interfaces. A "round trip" through a Python
   contour therefore clears that mark on any points that have it, and FontForge
   treats mid-placed and omitted :attr:`on_curve` points as equivalent.

.. attribute:: point.name

   The point name (generally there is no name)

.. method:: point.dup()

   Returns a copy of the current point.

.. method:: point.transform(tuple)

   Transforms the point by the transformation matrix

.. method:: point.__reduce__()

   This function allows the pickler to work on this type. I don't think it is
   useful for anything else.


Contour
-------

A contour is a collection of points. A contour may be either based on cubic or
quadratic splines.

If based on cubic splines there should be either 0 or 2 off-curve points
between every two on-curve points. If there are no off-curve points then
we have a line between those two points. If there are 2 off-curve points
we have a cubic bezier curve between the two end points.

If based on quadratic splines things are more complex. Again, two
adjacent on-curve points yield a line between those points. Two on-curve
points with an off-curve point between them yields a quadratic bezier
curve. However if there are two adjacent off-curve points then an
on-curve point will be interpolated between them. (This should be
familiar to anyone who has read the truetype 'glyf' table docs).

For examples of what these splines can look like see the
:doc:`section on bezier curves </techref/bezier>`.

A contour may be open in which case it is just a long wiggly line, or
closed when it is more like a circle with an inside and an outside.
Unless you are making stroked fonts all your contours should eventually
be closed.

Contours may also be expressed in terms of Raph Levien's spiro points.
This is an alternate representation for the contour, and is not always
available (Only if :func:`fontforge.hasSpiro()` is ``True``. If
available the spiro member will return a tuple of spiro control points,
while assigning to this member will change the shape of the contour to
match the new spiros.

Two contours may be compared to see if they describe similar paths.



.. class:: contour(is_quadratic=False)

   Creates a new contour.

.. attribute:: contour.is_quadratic

   Whether the contour should be interpretted as a set of quadratic or cubic
   splines. Setting this value has the side effect of converting the point list
   to the appropriate format.

.. attribute:: contour.closed

   Whether the contour is open or closed.

.. attribute:: contour.name

   The contour name (generally there is no name).

.. attribute:: contour.spiros

   This is an alternate representation of a curve. This member is only
   available if :meth:`fontforge.hasSpiro()` is ``True``. Returns a tuple
   of spiro control points. Each of these is itself a tuple of four
   elements; an x,y location, a type field, and a set of flags. The type
   field takes on values (which are predefined constants in the
   :mod:`fontforge` module):



   * :data:`fontforge.spiroG4`
   * :data:`fontforge.spiroG2`
   * :data:`fontforge.spiroCorner`
   * :data:`fontforge.spiroLeft`
   * :data:`fontforge.spiroRight`
   * :data:`fontforge.spiroOpen`

   For more information on what these point types mean see
   `Raph Levien's work <https://www.levien.com/spiro/>`_.

   The flags argument is treated as a bitmap of which currently on one bit (0x1)
   is defined. This indicates that this point is selected in the UI.

   When you assign a tuple of spiro control points to this member, the point
   list for the Bezier interpretation of the contour will change. And when you
   change the Bezier interpretation the set of spiro points will change.

.. method:: contour.dup()

   Returns a deep copy of the contour. That is, it copies the points that make
   up the contour.

.. method:: contour.isEmpty()

   Returns whether the contour is empty (contains no points)

.. method:: contour.boundingBox()

   Returns a tuple representing a rectangle ``(xmin,ymin, xmax,ymax)`` into
   which the contour fits. It is not guaranteed to be the smallest such
   rectangle, but it will often be.

.. method:: contour.getSplineAfterPoint(pos)

   Returns a tuple of two four-element tuples. These tuples are x and y splines
   for the curve after the specified point.

.. method:: contour.draw(pen)

   Draw the contour to the pen argument.

.. method:: contour.__reduce__()

   This function allows the pickler to work on this type. I don't think it is
   useful for anything else.

.. method:: contour.__iter__()

   Returns an iterator for the contour which will return the points in order.


.. rubric:: Sequence Protocol

Does not support the repeat concept.

.. object:: len(c)

   The number of points in the contour

.. object:: c[i]

   The ``i`` th point on the contour. You may assign a point or point
   initializer to this (keeping the number of points constant) or use
   ``del c[i]`` to remove the entry (reducing the number by one).

.. object:: c[i:j]

   The contour containing points between i and j; i must be >= j.
   Alternatively, ``c[j:i:-1]`` returns those points in reverse order (larger
   strides are not supported).

.. object:: c[i:j] = d

   The points between i and j are replaced by those in d; i must be >= j.
   d can be a contour or a sequence of point initializer tuples, as in
   ``[(1,1,False),(2,1)]``. If ``c[j:i:-1]`` is used instead the points of d
   are assigned in reverse order.

.. object:: c + d

   A contour concatenating c and d. d may be or encode either another contour
   or a point.

.. object:: c += d

   Appends d to c. d may be or encode either another contour or a point.

.. object:: p in c

   When p is a point, returns whether some point ``(p.x, p.y)`` is in the
   contour c. p can also be a tuple of two numbers.


.. rubric:: Contour construction

.. method:: contour.moveTo(x, y)

   Adds an initial, on-curve point at ``(x,y)`` to the contour

.. method:: contour.lineTo(x, y[, pos])

   Adds an line to the contour. If the optional third argument is give, the
   line will be added after the pos'th point, otherwise it will be at the end
   of the contour.

.. method:: contour.cubicTo((cp1x, cp1y)(cp2x, cp2y)(x, y)[, pos])

   Adds a cubic curve to the contour. If the optional third argument is give,
   the line will be added after the pos'th point, otherwise it will be at the
   end of the contour.

.. method:: contour.quadraticTo((cpx, cpy)(x, y)[, pos])

   Adds a quadratic curve to the contour. If the optional third argument is
   give, the line will be added after the pos'th point, otherwise it will be at
   the end of the contour.

.. method:: contour.insertPoint(point[, pos])

   Adds point to the contour. If the optional third argument is give, the line
   will be added after the pos'th point, otherwise it will be at the end of the
   contour. The point may be either a point or a point initializer tuple.

.. method:: contour.makeFirst(pos)

   Rotate the point list so that the pos'th point becomes the first point

.. method:: contour.isClockwise()

   Returns whether the contour is drawn in a clockwise direction. A return
   value of -1 indicates that no consistant direction could be found (the
   contour self-intersects).

.. method:: contour.reverseDirection()

   Reverse the order in which the contour is drawn (turns a clockwise contour
   into a counter-clockwise one). See also :meth:`layer.correctDirection`.

.. method:: contour.similar(other_contour[, error])

   Checks whether this contour is similar to the other one where error is the
   maximum distance (in em-units) allowed for the two contours to diverge.

   This is like the comparison operator, but that doesn't allow you to specify
   an error bound.

.. method:: contour.xBoundsAtY(ybottom[, ytop])

   Finds the minimum and maximum x positions attained by the contour when y is
   between ybottom and ytop (if ytop is not specified it is assumed the same as
   ybottom). If the contour does not have any y values in the specified range
   then ff will return ``None``.

.. method:: contour.yBoundsAtX(xleft[, xright])

   Finds the minimum and maximum y positions attained by the contour when x is
   between xleft and xright (if xright is not specified it is assumed the same
   as xleft). If the contour does not have any x values in the specified range
   then ff will return ``None``.


.. rubric:: Contour manipulation

.. method:: contour.addExtrema([flags, emsize])

   Extrema should be marked by on-curve points. If a curve lacks a point at an
   extrema this command will add one. Flags may be one of the following strings:

   .. object:: all

      Add all missing extrema

   .. object:: only_good

      Only add extrema on longer splines (with respect to the em-size)

   .. object:: only_good_rm

      As above but also merge away on-curve points which are very close to, but
      not on, an added extremum

.. method:: contour.cluster([within, max])

   Moves clustered coordinates to a standard central value.

   See also :meth:`contour.round()`.

.. method:: contour.merge(pos)

   Removes the on-curve point a the given position and rearranges the other
   points to make the curve as similar to the original as possible. (pos may
   also be a tuple of positions, all of which will be removed)

   See also :meth:`contour.simplify()`.

.. method:: contour.round([factor])

   Rounds the x and y coordinates. If factor is specified then ::

      new_coord = round(factor*old_coord)/factor

   See also :meth:`contour.cluster()`.

.. method:: contour.selfIntersects()

   Returns whether this contour intersects itself.

.. method:: contour.simplify([error_bound, flags, tan_bounds, linefixup, linelenmax])

   Tries to remove excess points on the contour if doing so will not perturb
   the curve by more than error-bound. Flags is a tuple of the following strings:

   .. object:: ignoreslopes

      Allow slopes to change

   .. object:: ignoreextrema

      Allow removal of extrema

   .. object:: smoothcurves

      Allow curve smoothing

   .. object:: choosehv

      Snap to horizontal or vertical

   .. object:: forcelines

      flatten bumps on lines

   .. object:: nearlyhvlines

      Make nearly horizontal/vertical lines be so

   .. object:: mergelines

      Merge adjacent lines into one

   .. object:: setstarttoextremum

      Rotate the point list so that the start point is on an extremum

   .. object:: removesingletonpoints

      If the contour contains just one point then remove it

   See also :meth:`contour.merge()`.

.. method:: contour.transform(matrix)

   Transforms the contour by the matrix


Layer
-----

A layer is a collection of contours. All the contours must be the same order
(all quadratic or all cubic). Currently layers do not contain references.

Layers may be compared to see if their contours are similar.

.. class:: layer()

   Creates a new layer

.. method:: layer.is_quadratic()

   Whether the contours should be interpretted as a set of quadratic cubic
   splines. Setting this value has the side effect of converting the contour
   list to the appropriate format.

.. method:: layer.__iter__()

   Returns an iterator for the layer which will return the contours in order.

.. method:: layer.__reduce__()

   This function allows the pickler to work on this type. I don't think it is
   useful for anything else.

.. method:: layer.dup()

   Returns a deep copy of the layer. That is, it will copy all the contours and
   all the points as well as copying the layer object itself.

.. method:: layer.isEmpty()

   Returns whether the layer is empty (contains no contour)

.. method:: layer.addExtrema([flags, emsize])

   Extrema should be marked by on-curve points. If a curve lacks a point at an
   extrema this command will add one. Flags may be one of the following strings:

   .. object:: all

      Add all missing extrema

   .. object:: only_good

      Only add extrema on longer splines (with respect to the em-size)

   .. object:: only_good_rm

      As above but also merge away on-curve points which are very close to, but
      not on, an added extremum

.. method:: layer.cluster([within, max])

   Moves clustered coordinates to a standard central value.

   See also :meth:`layer.round()`.

.. method:: layer.correctDirection()

   Orients all contours so that external ones are clockwise and internal
   counter-clockwise. See also :meth:`contour.isClockwise()`.

.. method:: layer.export(filename[, KEYWORD])

   Exports the current layer (in outline format) to a file. The type of file is
   determined by the extension.

   The following optional keywords modify the export process for various formats: 

   .. object:: usetransform (boolean, default=False)

      Flip the Y-axis of exported SVGs with a transform element rather than
      modifying the individual Y values. 

   .. object:: usesystem (boolean, default=False)

      Ignore the above keyword settings and use the values set by the user
      in the Import options dialog. 

   .. object:: asksystem (boolean, default=False)

      If the UI is present show the Import options dialog to the user
      and use the chosen values (does nothing otherwise).

.. method:: layer.exclude(excluded_layer)

   Removes the excluded area from the current contours. See also
   :meth:`layer.removeOverlap()` and :meth:`layer.intersect()`.

.. method:: layer.intersect()

   Leaves only areas in the intersection of contours. See also
   :meth:`layer.removeOverlap()` and :meth:`layer.exclude()`.

.. method:: layer.removeOverlap()

   Removes overlapping areas. See also :meth:`layer.intersect()` and
   :meth:`layer.exclude()`.

.. method:: layer.interpolateNewLayer(other_layer, amount)

   Creates (and returns) a new layer which contains splines interpolated from
   the current layer and the first argument. If amount is 0 the result will
   look like the current layer, if 1 then like the first argument.

.. method:: layer.round([factor])

   Rounds the x and y coordinates. If factor is specified then ::

      new_coord = round(factor*old_coord)/factor

   See also :meth:`layer.cluster()`.

.. method:: layer.selfIntersects()

   Returns whether any of the contours on this layer intersects any other
   contour (including itself).

.. method:: layer.similar(other_layer[, error])

   Checks whether this layer is similar to the other one where error is the
   maximum distance (in em-units) allowed for any two corresponding contours
   in the layers to diverge.

   This is like the comparison operator, but that doesn't allow you to specify
   an error bound.

.. method:: layer.simplify([error_bound, flags, tan_bounds, linefixup, linelenmax])

   Tries to remove excess points on the layer if doing so will not perturb the
   curve by more than error-bound. Flags is a tuple of the following strings:

   .. object:: ignoreslopes

      Allow slopes to change

   .. object:: ignoreextrema

      Allow removal of extrema

   .. object:: smoothcurves

      Allow curve smoothing

   .. object:: choosehv

      Snap to horizontal or vertical

   .. object:: forcelines

      flatten bumps on lines

   .. object:: nearlyhvlines

      Make nearly horizontal/vertical lines be so

   .. object:: mergelines

      Merge adjacent lines into one

   .. object:: setstarttoextremum

      Rotate the point list so that the start point is on an extremum

   .. object:: removesingletonpoints

      If the contour contains just one point then remove it

.. method:: layer.stemControl(stem_width_scale, [hscale, stem_height_scale, vscale, xheight])

   Allows you to scale counters and stems independently of each other.
   ``stem_width_scale`` specifies by how much the widths of stems should be
   scaled (this should be a number around 1).

   If omitted, ``hscale`` defaults to 1, otherwise it will indicate the
   horizontal scaling factor for the glyph as a whole.

   If omitted, ``stem_height_scale`` defaults to ``stem_width_scale``,
   otherwise it specifies the scaling for stem heights.

   If omitted, ``vscale`` defaults to ``hscale``, otherwise it specifies the
   vertical scale factor for the glyph as a whole. ``xheight`` is optional; if
   specified it will fix the points at that height so that they will be at the
   same level across glyphs.

.. method:: layer.stroke("circular", width[, CAP, JOIN, FLAGS])
            layer.stroke("elliptical", width, minor_width, ANGLE  [, CAP, JOIN, FLAGS])
            layer.stroke("calligraphic", width, height, angle[, FLAGS])
            layer.stroke("polygon", contour[, FLAGS])
   (Legacy interface)
   :noindex:

.. method:: layer.stroke("circular", width  [, CAP, JOIN, ANGLE, KEYWORD])
            layer.stroke("elliptical", width, minor_width  [, ANGLE, CAP, JOIN, KEYWORD])
            layer.stroke("calligraphic", width, height  [, ANGLE, CAP, JOIN, KEYWORD])
            layer.stroke("convex", contour[, ANGLE, CAP, JOIN, KEYWORD])
   (Current interface)

   Strokes the lines of each contour in the layer according to the supplied
   parameters. See the corresponding :meth:`glyph.stroke()` for a description
   of the syntax and the :doc:`stroke </techref/stroke>` documentation for more general
   information.

.. method:: layer.transform(matrix)

   Transforms the layer by the matrix

.. method:: layer.nltransform(xexpr, yexpr)

   xexpr and yexpr are strings specifying non-linear transformations that will
   be applied to all points in the layer (with xexpr being applied to x values,
   and yexpr to y values, of course). The syntax for the expressions is
   explained in the :ref:`non-linear transform dialog <transform.Non-Linear>`.

.. method:: layer.boundingBox()

   Returns a tuple representing a rectangle ``(xmin,ymin, xmax,ymax)`` into
   which the layer fits. It is not guaranteed to be the smallest such
   rectangle, but it will often be.

.. method:: layer.xBoundsAtY(ybottom[, ytop])

   Finds the minimum and maximum x positions attained by the contour when y is
   between ybottom and ytop (if ytop is not specified it is assumed the same as
   ybottom). If the layer does not have any y values in the specified range
   then FontForge will return ``None``.

.. method:: layer.yBoundsAtX(xleft[, xright])

   Finds the minimum and maximum y positions attained by the contour when x is
   between xleft and xright (if xright is not specified it is assumed the same
   as xleft). If the layer does not have any x values in the specified range
   then FontForge will return ``None``.

.. method:: layer.draw(pen)

   Draw the layer to the :class:`pen <glyphPen>` argument.


.. rubric:: Sequence Protocol

Does not support the repeat, slice and contains concepts.

.. object:: len(l)

   The number of contours in the layer

.. object:: l[i]

   The ``i`` th contour in the layer. You may assign a contour to this (keeping
   the number of contours constant) or use ``del l[i]`` to remove the entry
   (reducing the number by one)

.. object:: l + m

   A layer concatenating l and m. m may be either another layer or a contour.

.. object:: l += m

   Appends m to l. m may be either another layer or a contour.


Glyph Pen
---------

This implements the `Pen Protocol <http://robofab.org/objects/pens.html>`_ to
draw a FontForge :class:`glyph`. You create a :class:`glyphPen` with
:meth:`glyph.glyphPen()`. You then draw into it with the operators below.

This type may not be pickled.

.. rubric:: Example

::

   import fontforge
   font = fontforge.open("Ambrosia.sfd") # Open a font
   pen = font["B"].glyphPen()            # Create a pen to draw into glyph "B"
   pen.moveTo((100,100))                 # draw a square
   pen.lineTo((100,200))
   pen.lineTo((200,200))
   pen.lineTo((200,100))
   pen.closePath()                       # end the contour

   font["A"].draw(pen)                   # or you can copy from one glyph to another
                                         # by having a glyph draw itself into the pen
   pen = None                            # Finalize the pen. This tells FontForge
                                         # that the drawing is done and causes
                                         # it to refresh the display (if a UI is active).


.. class:: glyphPen

.. method:: glyphPen.moveTo((x, y))

   With one exception this call begins every contor and creates an on curve
   point at ``(x,y)`` as the start point of that contour. This should be the
   first call after a pen has been created and the call that follows a
   :meth:`glyphPen.closePath()`, :meth:`glyphPen.endPath()`.

.. method:: glyphPen.lineTo((x, y))

   Draws a line from the last point to ``(x,y)`` and adds that to the contour.

.. method:: glyphPen.curveTo((cp1.x, cp1.y), (cp2.x, cp2.y), (x, y)) ((cp.x, cp.y), (x, y))

   This routine has slightly different arguments depending on the type of the
   font. When drawing into a cubic font (PostScript) use the first set of
   arguments (with two control points -- off curve points -- between each on
   curve point). When drawing into a quadratic font (TrueType) use the second
   format with one control point between adjacent on-curve points.

   The standard appears to support super-bezier curves with more than two
   control points between on-curve points. FontForge does not. Nor does
   FontForge allow you to draw a quadratic spline into a cubic font, nor vice versa.

.. method:: glyphPen.qCurveTo([(cp.x, cp.y)]*, (x, y)) ([(cp.x, cp.y)]*, None))

   This routine may only be used in quadratic (TrueType) fonts and has two
   different formats. It is used to express the TrueType idiom where an on-curve
   point mid-way between its control points may be omitted, leading to a run of
   off-curve points (with implied but unspecified on-curve points between them).

   The first format allows an arbetary number of off-curve points followed by
   one on-curve point.

   It is possible to have a contour which consists solely of off-curve points.
   When this happens the contour is NOT started with a :meth:`glyphPen.moveTo()`,
   instead the entire contour, all the off curve points, are listed in one call,
   and the argument list is terminated by a ``None`` to indicate there are no
   on-curve points.

.. method:: glyphPen.closePath()

   Closes the contour (connects the last point to the first point to make a
   loop) and ends it.

.. method:: glyphPen.endPath()

   Ends the contour without closing it. This is only relevant if you are
   stroking contours.

.. method:: glyphPen.addComponent(glyph_name, transform)

   Adds a reference (a component) to the glyph. The PostScript transformation
   matrix is a 6 element tuple.


Glyph
-----

The glyph type refers to a :class:`glyph` object. It has no independent life
of its own, it always lives within a font. It has all the things you expect to
be associated with a glyph: a glyph name, a unicode encoding, a drawing layer,
GPOS/GSUB features...


This type may not be pickled.

This type may not be created directly -- all glyphs are bound to a font and
must be created through the font.

.. class:: glyph

   **Note:** Glyphs do not have an independent existence. They live in fonts.
   You may not create them stand-alone, only in the context of a font. See
   :meth:`font.createChar()`.

.. attribute:: glyph.activeLayer

   Returns currently active layer in the glyph (as an integer). May be set to
   an integer or a layer name to change the active layer.

.. attribute:: glyph.altuni

   Returns additional unicode code points for this glyph. For a primary code
   point, see :attr:`glyph.unicode` .

   Returns either None or a tuple of alternate encodings. Each alternate
   encoding is a tuple of ::

   (unicode-value, variation-selector, reserved-field)

   The first is an unicode value of this alternate code point. The second is an
   integer for variation selector and can be set to -1 if not used. The third
   is an empty field reserved for future use and currently must be set to zero.

   :attr:`glyph.altuni` can be set to None to clear all alternates, or to a
   tuple. The elements of the tuple may be either integers (an alternate
   unicode value with no variation selector) or a tuple with up to 3 values in
   it as explained above.

.. attribute:: glyph.anchorPoints

   Returns the list of anchor points in the glyph. Each anchor point is a
   tuple of ::

      (anchor-class-name, type, x,y [,ligature-index])

   The first two are strings, the next two doubles, and the last (which is only
   present if ``type=="ligature"``) is an integer. Type may be

   * ``mark``
   * ``base``
   * ``ligature``
   * ``basemark``
   * ``entry``
   * ``exit``

.. attribute:: glyph.anchorPointsWithSel

   Same as the above, except also includes whether the anchor point is selected
   in the UI. Returns a tuple of all anchor points in the glyph. Each anchor
   point is a tuple of ::

   (anchor-class-name, type, x,y, selected [,ligature-index])

   The first two are strings, the next two doubles, then a boolean, and the
   last (which is only present if ``type=="ligature"``) is an integer.
   Type may be

   * ``mark``
   * ``base``
   * ``ligature``
   * ``basemark``
   * ``entry``
   * ``exit``

.. attribute:: glyph.background

   The glyph's background layer. This is a *copy* of the glyph's data. See
   also :attr:`glyph.foreground` and :attr:`glyph.layers`.

.. attribute:: glyph.changed

   Whether this glyph has been modified. This is (should be) maintained
   automatically, but you may set it if you wish.

.. attribute:: glyph.color

   The color of the glyph in the fontview. A 6 hex-digit RGB number or -1 for
   default. 0xffffff is white, 0x0000ff is blue, etc.

.. attribute:: glyph.comment

   Any comment you wish to associate with the glyph. UTF-8

.. attribute:: glyph.dhints

   A tuple with one entry for each diagonal stem hint. Each stem hint is itself
   represented by a tuple of three coordinate pairs (themselves tuples of two
   numbers), these three are: a point on one side of the stem, a point on the
   other side, and a unit vector pointing in the stem's direction.

.. attribute:: glyph.encoding

   Returns the glyph's encoding in the font's encoding. (readonly)

   If the glyph has multiple encodings, one will be picked at random.

   If the glyph is not in the font's encoding then a number will be returned
   beyond the encoding size (or in some cases -1 will be returned).

.. attribute:: glyph.font

   The font containing this glyph. (readonly)

.. attribute:: glyph.foreground

   The glyph's foreground layer. This is a *copy* of the glyph's data. See also
   :attr:`glyph.background`, :attr:`glyph.layers` and :attr:`glyph.references`.

.. attribute:: glyph.glyphclass

   An opentype glyphclass, one of automatic, noclass, baseglyph, baseligature,
   mark, component

.. attribute:: glyph.glyphname

   The name of the glyph

.. attribute:: glyph.hhints

   A tuple of all horizontal postscript hints. Each hint is itself a tuple of
   starting locations and widths.

.. attribute:: glyph.horizontalComponents

   A tuple of tuples.

   This allows :ref:`constructing <math.GlyphConstruction>` very large versions
   of the glyph by stacking the componants together. Some components may be
   repeated so there is no bound on the size.

   This is different from horizontalVariants which expects prebuilt glyphs of
   various fixed sizes.

   The components are stacked in the order they appear in the (top-level) tuple.
   Each sub-tuple represents information on one component. The subtuple should
   contain: (String glyph-name, Boolean is-extender, Int startConnectorLength,
   Int endConnectorLength, Int fullAdvance). Any of these may be omitted (except
   the glyph name) and will be assumed to be 0 if so.

.. attribute:: glyph.horizontalComponentItalicCorrection

   The italic correction for any composite glyph made with the horizontalComponents.

.. attribute:: glyph.horizontalVariants

   A string containing a list of glyph names. These are
   :ref:`alternate forms <math.Variants>` of the current glyph for use in
   typesetting math. Presumably the variants are of different sizes.

   Although ff will always return a string of glyph names, you may assign to it
   with a tuple of glyphs and ff will convert that to corresponding names.

.. attribute:: glyph.isExtendedShape

   A boolean containing the MATH "is extended shape" field.

.. attribute:: glyph.italicCorrection

   The glyph's italic correction field. Used by both TeX and MATH. The special
   value :data:`fontforge.unspecifiedMathValue` means the value is unspecified
   (An unspecified value will not go into the output tables, a value of 0 will)

.. attribute:: glyph.layer_cnt

   The number of layers in this glyph. (Cannot be set)

.. attribute:: glyph.layers

   A dictionary like object containing the layers of the glyph. It may be
   indexed by either a layer name, or an integer between 0 and
   ``glyph.layer_cnt-1`` to produce a :class:`layer` object. Layer 0 is the
   background layer. Layer 1 is the foreground layer.

.. attribute:: glyph.layerrefs

   A dictionary like object containing the references in the layers of the
   glyph. It may be indexed by either a layer name, or an integer between 0 and
   ``glyph.layer_cnt-1`` to produce a :attr:`reference tuple<glyph.references>`
   object. Layer 0 is the background layer. Layer 1 is the foreground layer.

.. attribute:: glyph.lcarets

   A tuple containing the glyph's ligature caret locations. Setting this will
   also either enable or disable the "Default Ligature Caret Count" flag
   depending from the number of elements in the tuple.

.. attribute:: glyph.left_side_bearing

   The left side bearing of the glyph. Setting this value will adjust all
   layers so that guides in the background etc will be adjusted with the rest
   of the glyph

.. attribute:: glyph.manualHints

   The glyph's hints have been set by hand, and the glyph should not be
   autohinted without a specific request from the user. The "Don't AutoHint" flag.

.. attribute:: glyph.mathKern.bottomLeft

   The glyph's math kerning data associated with the bottom left vertex. This
   returns a tuple of two element tuples, each of which contains a kerning
   offset and an associated height (in the last entry the height term is
   meaningless, but present).

.. attribute:: glyph.mathKern.bottomRight

   The glyph's math kerning data associated with the bottom right vertex. This
   returns a tuple of two element tuples, each of which contains a kerning
   offset and an associated height (in the last entry the height term is
   meaningless, but present).

.. attribute:: glyph.mathKern.topLeft

   The glyph's math kerning data associated with the top left vertex. This
   returns a tuple of two element tuples, each of which contains a kerning
   offset and an associated height (in the last entry the height term is
   meaningless, but present).

.. attribute:: glyph.mathKern.topRight

   The glyph's math kerning data associated with the top right vertex. This
   returns a tuple of two element tuples, each of which contains a kerning
   offset and an associated height (in the last entry the height term is
   meaningless, but present).

.. attribute:: glyph.originalgid

   The GID of this glyph in the font it was read from. (readonly)

.. attribute:: glyph.persistent

   Whatever you want (these data will be saved as a pickled object in the
   sfd file. It is your job to insure that whatever you put here can be pickled).
   See also the :attr:`glyph.temporary` field.

.. attribute:: glyph.references

   A tuple of tuples containing glyph-name and a transformation matrix for each
   reference in the foreground. See also :attr:`glyph.foreground` and :attr:`glyph.layerrefs`.

.. attribute:: glyph.right_side_bearing

   The right side bearing of the glyph

.. attribute:: glyph.script

   A string containing the OpenType 4 letter tag for the script associated with
   this glyph (readonly)

.. attribute:: glyph.temporary

   Whatever you want (these data will be lost once the font is closed)

   See also :attr:`glyph.persistent`.

.. attribute:: glyph.texheight

   The Tex height. The special value :data:`fontforge.unspecifiedMathValue`
   means the field is unspecified (An unspecified value will not go into the
   output tables, a value of 0 will)

.. attribute:: glyph.texdepth

   The Tex depth. The special value :data:`fontforge.unspecifiedMathValue`
   means the field is unspecified (An unspecified value will not go into the
   output tables, a value of 0 will)

.. attribute:: glyph.topaccent

   The glyph's top accent position field. Used by MATH. The special value
   :data:`fontforge.unspecifiedMathValue` means the field is unspecified (An
   unspecified value will not go into the output tables, a value of 0 will)

.. attribute:: glyph.ttinstrs

   Any truetype instructions, returned as a binary string

.. attribute:: glyph.unicode

   The glyph's unicode code point, or -1. In addition to this primary mapping,
   a glyph can have multiple secondary mappings - see :attr:`glyph.altuni`.

.. attribute:: glyph.unlinkRmOvrlpSave

   A flag that indicates the glyph's references should be unlinked and remove
   overlap run on it before the font is saved (and then the original references
   replaced after the save finishes)

.. attribute:: glyph.user_decomp

   Your preferred decomposition for this glyph; used by :meth:`glyph.build()`.

.. attribute:: glyph.userdata

   .. warning:: Deprecated name for :attr:`glyph.temporary`

.. attribute:: glyph.vhints

   A tuple of all vertical postscript hints. Each hint is itself a tuple of
   starting locations and widths.

.. attribute:: glyph.validation_state

   A bit mask indicating some problems this glyph might have. (readonly)

   .. object:: 0x1

      If set then this glyph has been validated.

      If unset then other bits are meaningless.

   .. object:: 0x2

      Glyph has an open contour.

   .. object:: 0x4

      Glyph intersects itself somewhere.

   .. object:: 0x8

      At least one contour is drawn in the wrong direction

   .. object:: 0x10

      At least one reference in the glyph has been flipped

      (and so is drawn in the wrong direction)

   .. object:: 0x20

      Missing extrema

   .. object:: 0x40

      A glyph name referred to from this glyph, in an opentype table, is not
      present in the font.

   .. object:: 0x40000

      Points (or control points) are too far apart. (Coordinates must be
      within 32767)

   **Postscript only**

   .. object:: 0x80

      PostScript has a limit of 1500 points in a glyph.

   .. object:: 0x100

      PostScript has a limit of 96 hints in a glyph.

   .. object:: 0x200

      Invalid glyph name.

   **TrueType only, errors in original file**

   .. object:: 0x400

      More points in a glyph than allowed in 'maxp'

   .. object:: 0x800

      More paths in a glyph than allowed in 'maxp'

   .. object:: 0x1000

      More points in a composite glyph than allowed in 'maxp'

   .. object:: 0x2000

      More paths in a composite glyph than allowed in 'maxp'

   .. object:: 0x4000

      Instructions longer than allowed in 'maxp'

   .. object:: 0x8000

      More references in a glyph than allowed in 'maxp'

   .. object:: 0x10000

      References nested more deeply than allowed in 'maxp'

   .. object:: 0x40000

      Points too far apart. TrueType and Type2 fonts are limited to 16 bit
      numbers, and so adjacent points must be within 32767 em-units of each other.

   .. object:: 0x80000

      Points non-integral. TrueType points and control points must be integer
      aligned. (FontForge will round them if they aren't)

   .. object:: 0x100000

      Missing anchor. According to the opentype spec, if a glyph contains an
      anchor point for one anchor class in a subtable, it must contain anchor
      points for all anchor classes in the subtable. Even it, logically, they
      do not apply and are unnecessary.

   .. object:: 0x200000

      Duplicate glyph name. Two (or more) glyphs in this font have the same
      name. When outputting a PostScript font only one of them will ever be seen.

      It's a little hard to detect this in normal use, but if you change the
      encoding to "Glyph Order", and then use Edit->Select->Wildcard and enter
      the glyph name, both of them should be selected.

   .. object:: 0x400000

      Duplicate unicode code point. Two (or more) glyphs in this font have the
      code point. When outputting an sfnt (TrueType/OpenType) font only one of
      them will ever be seen.

      It's a little hard to detect this in normal use, but if you change the
      encoding to "Glyph Order", and then use Edit->Select->Wildcard and enter
      the code point, both of them should be selected.

   .. object:: 0x800000

      Overlapped hints. Either the glyph has no hint masks and there are
      overlapped hints, or a hint mask specifies two overlapping hints.

.. attribute:: glyph.verticalComponents

   A tuple of tuples.

   This allows :ref:`constructing <math.GlyphConstruction>` very large versions
   of the glyph by stacking the componants together. Some components may be
   repeated so there is no bound on the size.

   This is different from verticalVariants which expects prebuilt glyphs of
   various fixed sizes.

   The components are stacked in the order they appear in the (top-level) tuple.
   Each sub-tuple represents information on one component. The subtuple should
   contain: (String glyph-name, Boolean is-extender, Int startConnectorLength,
   Int endConnectorLength, Int fullAdvance). Any of these may be omitted
   (except the glyph name) and will be assumed to be 0 if so.

.. attribute:: glyph.verticalComponentItalicCorrection

   The italic correction for any composite glyph made with the verticalComponents.

.. attribute:: glyph.verticalVariants

   A string containing a list of glyph names. These are
   :ref:`alternate forms <math.Variants>`
   of the current glyph for use in typesetting math. Presumably the variants
   are of different sizes.

.. attribute:: glyph.width

   The advance width of the glyph. See also :attr:`glyph.vwidth`.

.. attribute:: glyph.vwidth

   The vertical advance width of the glyph. See also :attr:`glyph.width`.

.. method:: glyph.addAnchorPoint(anchor_class_name, anchor_type, x, y[, ligature_index])

   Adds an anchor point. anchor-type may be one of the strings

   * ``"mark"``
   * ``"base"``
   * ``"ligature"``
   * ``"basemark"``
   * ``"entry"``
   * ``"exit"``


   If there is an anchor point with the same ``anchor_class_name`` and:

   * lookup type is ``"gpos_mark2base"`` or
   * lookup type is ``"gpos_mark2ligature"`` and ``ligature_index`` is the same or
   * ``anchor_type`` is the same

   then the existing anchor will be overwritten.

.. method:: glyph.addExtrema([flags, emsize])

   Extrema should be marked by on-curve points. If a curve lacks a point at an
   extrema this command will add one. Flags may be one of the following strings

   .. object:: all

      Add all missing extrema

   .. object:: only_good

      Only add extrema on longer splines (with respect to the em-size)

   .. object:: only_good_rm

      As above but also merge away on-curve points which are very close to,
      but not on, an added extremum

.. method:: glyph.addReference(glyph_name[, transform])

   Adds a reference to the specified glyph into the current glyph. Optionally
   specifying a transformation matrix

.. method:: glyph.addHint(is_vertical, start, width)

   Adds a postscript hint. Takes a boolean flag indicating whether the hint is
   horizontal or vertical, a start location and the hint's width.

.. method:: glyph.addPosSub(subtable_name, variant)
            glyph.addPosSub(subtable_name, variants)
            glyph.addPosSub(subtable_name, ligature_components)
            glyph.addPosSub(subtable_name, xoff, yoff, xadv, yadv)
            glyph.addPosSub(subtable_name, other_glyph_name, kerning)
            glyph.addPosSub(subtable_name, other_glyph_name, xoff1, yoff1, xadv1, yadv1, xoff2, yoff2, xadv2, yadv2)

   Adds position/substitution data to the glyph. The number and type of the
   arguments vary acording to the type of the lookup containing the subtable.

   The first argument should always be a lookup subtable name.

   If the lookup is for single substitutions then the second argument should be
   a string containing a single glyph name.

   For multiple and alternated substitutions a tuple of glyph names. For
   ligatures, a tuple of the ligature components (glyph names).

   For single positionings the second through fifth arguments should be small
   integers representing the adjustment along the appropriate axis.

   For pairwise positionings (kerning) the second argument should be the name
   of the other glyph being kerned with, and the third through tenth should be
   small integers -- or, if there are exactly three arguments then the third
   specifies traditional, one-axis, kerning.

   If there is a previously existing entry, this will replace it (except for
   ligatures).

.. method:: glyph.appendAccent(name="glyph_name")
            glyph.appendAccent(unicode=<codepoint>)

   Makes a reference to the specified glyph, adds that reference to the current
   layer of this glyph, and positions it to make a reasonable accent.

.. method:: glyph.autoHint()

   Generates PostScript hints for this glyph.

.. method:: glyph.autoInstr()

   Generates TrueType instructions for this glyph.

.. method:: glyph.autoTrace()

   Auto traces any background images

.. method:: glyph.boundingBox()

   Returns a tuple representing a rectangle (xmin,ymin, xmax,ymax) which is
   the minimum bounding box of the glyph.

.. method:: glyph.build()

   If the character is a composite character, then clears it and inserts
   references to its components.

.. method:: glyph.canonicalContours()

   Orders the contours in the current glyph by the x coordinate of their
   leftmost point. (This can reduce the size of the charstring needed to
   describe the glyph(s).

.. method:: glyph.canonicalStart()

   Sets the start point of all the contours of the current glyph to be the
   leftmost point on the contour. (If there are several points with that value
   then use the one which is closest to the baseline). This can reduce the size
   of the charstring needed to describe the glyph(s). By regularizing things it
   can also make more things available to be put in subroutines.

.. method:: glyph.changeWeight(stroke_width[, type, serif_height, serif_fuzz, counter_type, removeoverlap, custom_zones])

   See the :ref:`Element->Style->Change Width <Styles.Embolden>` command for a
   more complete description of these arguments.

   ``stroke_width`` is the amount by which all stems are expanded.

   ``type`` is one of ``"LCG"``, ``"CJK"``, ``"auto"``, ``"custom"``.

   ``serif_height`` tells ff not to expand serifs which are that much off the
   baseline, while serif_fuzz specifies the amount of fuzziness allowed in the
   match. If you don't want special serif behavior set this to 0.

   ``counter_type`` is one of ``"squish"``, ``"retain"``, ``"auto"``.

   ``removeoverlap`` (Cleanup Self Intersect) is a boolean int
   (0=false, 1=true). When activated, and FontForge detects that an expanded
   stroke will self-intersect, then setting this option will cause it to try to
   make things nice by removing the intersections.

   ``custom_zones`` is only meaningful if the type argument were ``"custom"``.
   It may be either a number, which specifies the "top hint" value (bottom hint
   is assumed to be 0, others are between), or a tuple of 4 numbers (top hint,
   top zone, bottom zone, bottom hint).

.. method:: glyph.condenseExtend(c_factor, c_add[, sb_factor, sb_add, correct])

   Condenses or extends the size of the counters and side-bearings of the glyph.
   The first two arguments provide information on shrinking/growing the
   counters, the second two the sidebearings. If the last two are omitted they
   default to the same values as the first two.

   A counter's width will become: ::

      new_width = c_factor * old_width + c_add

   If present the ``correct`` argument allows you to specify whether you want
   to correct for the italic angle before condensing the glyph.
   (it defaults to``True``)

.. method:: glyph.clear([layer])

   With no arguments, clears the contents of the glyph (and marks it as not :meth:`glyph.isWorthOutputting()`).
   It is not possible to clear the guide layer with this function.
   ``layer`` may be either an integer index or a string.

.. method:: glyph.cluster([within, max])

   Moves clustered coordinates to a standard central value.
   See also :meth:`glyph.round()`.

.. method:: glyph.correctDirection()

   Orients all contours so that external ones are clockwise and internal
   counter-clockwise.

.. method:: glyph.exclude(excluded_layer)

   Removes the excluded area from the current glyph. Takes an argument which is
   a layer. See also :meth:`glyph.removeOverlap()` and :meth:`glyph.intersect()`.

.. method:: glyph.export(filename[, KEYWORD])

   Creates a file with the specified name containing a representation of
   the glyph. Uses the file's extension to determine output file type.

   The following optional keywords modify the export process for various formats: 

   .. object:: layer (string or integer, default=glyph.activeLayer)

      For vector formats, the layer to export.

   .. object:: pixelsize (integer, default=100)

      For raster formats, the size of the image to output.

   .. object:: bitdepth (integer, default=8)

      For raster formats, the depth of the image to output. Must be 1 or 8.

   .. object:: usetransform (boolean, default=False)

      Flip the Y-axis of exported SVGs with a transform element rather than
      modifying the individual Y values. 

   .. object:: usesystem (boolean, default=False)

      Ignore the above keyword settings and use the values set by the user
      in the Import options dialog. 

   .. object:: asksystem (boolean, default=False)

      If the UI is present show the Import options dialog to the user
      and use the chosen values (does nothing otherwise).

   Note: The old positional layer/pixelsize,bitdepth calling conventions are
   still supported but are not compatible with the other keyword parameters.

.. method:: glyph.genericGlyphChange(stemType=<str>, thickThreshold=<double>, stemScale=<double>, stemAdd=<double>, stemHeightScale=<double>, stemHeightAdd=<double>, stemWidthScale=<double>, stemWidthAdd=<double>, thinStemScale=<double>, thinStemAdd=<double>, thickStemScale=<double>, thickStemAdd=<double>, processDiagonalStems=<boolean>, hCounterType=<str>, hCounterScale=<double>, hCounterAdd=<double>, lsbScale=<double>, lsbAdd=<double>, rsbScale=<double>, rsbAdd=<double>, vCounterType=<str>, vCounterScale=<double>, vCounterAdd=<double>, vScale=<double>, vMap=<tuple of tuples>)

   Similar to :meth:`font.genericGlyphChange`, but acting on this glyph only.

.. method:: glyph.getPosSub(lookup_subtable_name)

   Returns any positioning/substitution data attached to the glyph controlled
   by the lookup-subtable. If the name is ``"*"`` then returns data from all
   subtables.

   The data are returned as a tuple of tuples. The first element of the
   subtuples is the name of the lookup-subtable. The second element will be one
   of the strings: ``"Position"``, ``"Pair"``, ``"Substitution"``,
   ``"AltSubs"``, ``"MultSubs"``, ``"Ligature"``.

   Positioning data will be followed by four small integers representing
   adjustments to the: x position of the glyph, the y position, the horizontal
   advance, and the vertical advance.

   Pair data will be followed by the name of the other glyph in the pair and
   then eight small integers representing adjustments to the: x position of the
   first glyph, the y position, the horizontal advance, and the vertical
   advance, and then a similar foursome for the second glyph.

   Substitution data will be followed by a string containing the name of the
   glyph to replace the current one.

   Multiple and Alternate data will be followed by several strings each
   containing the name of a replacement glyph.

   Ligature data will be followed by several strings each containing the name
   of a ligature component glyph.

.. method:: glyph.importOutlines(filename, [KEYWORD])

   Uses the file's extension to determine behavior. Imports outline descriptions
   (eps, svg, glif files) into the forground layer. Imports image descriptions
   (bmp, png, xbm, etc.) into the background layer. The following optional keywords modify the import process for various formats:

   .. object:: scale (boolean, default=True)

      Scale imported images and SVGs to ascender height

   .. object:: simplify (boolean, default=True)

      Run simplify on the output of stroked paths

   .. object:: accuracy (float, default=0.25)

      The minimum accuracy (in em-units) of stroked paths.

   .. object:: default_joinlimit (float, default=-1)

      Override the format's default miterlimit for stroked paths, which is
      10.0 for PostScript and 4.0 for SVG. (Value -1 means "inherit" those
      defaults.)

   .. object:: handle_eraser (boolean, default=False)

      Certain programs use pens with white ink as erasers. When this flag is
      set FontForge will attempt to simulate that.

   .. object:: correctdir (boolean, default=False)

      Run "Correct direction" on (some) PostScript paths

   .. object:: usesystem (boolean, default=False)

      Ignore the above keyword settings and use the values set by the user
      in the Import options dialog. 

   .. object:: asksystem (boolean, default=False)

      If the UI is present show the Import options dialog to the user
      and use the chosen values (does nothing otherwise).

   Note: The old PostScript correctdir/handle_eraser flag tuple is still 
   supported but is not compatible with the other keywords. 

.. method:: glyph.intersect()

   Leaves only areas in the intersection of contours. See also
   :meth:`glyph.removeOverlap()` and :meth:`glyph.exclude()`.

.. method:: glyph.isWorthOutputting()

   Returns whether the glyph is worth outputting into a font file. Basically a
   glyph is worth outputting if it contains any contours, or references or has
   had its width set.

.. method:: glyph.preserveLayerAsUndo([layer, dohints])

   Normally undo handling is turned off during python scripting. If you wish
   you may tell fontforge to preserve the current state of a layer so that
   whatever you do later can be undone by the user. You may omit the layer
   parameter (in which case the currently active layer will be used). You may
   also request that hints be preserved (they are not, by default).

.. method:: glyph.removeOverlap()

   Removes overlapping areas.
   See also :meth:`glyph.intersect()` and :meth:`glyph.exclude()`.

.. method:: glyph.removePosSub(lookup_subtable_name)

   Removes all data from the glyph corresponding to the given lookup-subtable.
   If the name is "*" then all data will be removed.

.. method:: glyph.round([factor])

   Rounds the x and y coordinates of each point in the glyph. If factor is
   specified then ::

      new-coord = round(factor*old-coord)/factor

   See also :meth:`glyph.cluster()`.

.. method:: glyph.selfIntersects()

   Returns whether any of the contours in this glyph intersects any other
   contour in the glyph (including itself).

.. method:: glyph.setLayer(layer, layer_index[, flags])

   An alternative to assigning to :attr:`glyph.layers`, :attr:`glyph.background`,
   or :attr:`glyph.foreground`, and equivalent to those when not using the
   optional ``flags`` argument. When present, ``flags`` can be used to influence
   the types FontForge will assign to on-curve points. It should be a tuple of
   up to three of the following strings.

   (In the following descriptions *selected* refers to points picked out by the
   chosen ``select_`` flag, which is unrelated to :attr:`point.selected`. At
   most one ``"select_"`` flag and one mode flag should be included.)

   .. object:: select_none

      Each (on-curve) point will be assigned a type corresponding to its
      :attr:`point.type` value.

   .. object:: select_all

      (default) Each point will have a type assigned according to the chosen mode.

   .. object:: select_smooth

      Each point with the type :data:`splineCorner` will retain that type,
      others will be assigned a type according to the chosen mode. This makes
      :attr:`point.type` function like the ``smooth`` tag in the UFO glif
      format and some other spline storage formats.

   .. object:: select_incompat

      Each point with a type compatible with its current geometry will retain
      that type, others will be assigned a type according to the chosen mode.

   .. object:: by_geom

      (default) In this mode, each *selected* point will be assigned a type
      based on only its geometry. (However, see ``"hvcurve"``` below.)

   .. object:: downgrade

      In this mode, each *selected* point will be assigned the most specific
      type compatible with its geometry and its :attr:`point.type`. A point
      marked :data:`splineHVCurve` can keep that type or be downgraded to
      :data:`splineCurve` or :data:`splineCorner`, while a :data:`splineCurve`
      or :data:`splineTangent` can keep that (respective) type or be downgraded
      to :data:`splineCorner`. (:data:`splineCorner` is compatible with any
      geometry.)

   .. object:: check

      In this mode, the type of each *selected* point is verified to be
      compatible with its geometry. If it is not compatible the function raises
      an exception. (At present this exception is not very informative. However,
      to identify the specific problem one can duplicate the layer, use
      :meth:`glyph.setLayer()` with ``downgrade``, and then retrieve the layer
      and compare it with the original.)

   .. object:: force

      In this mode, the geometry of each *selected* point is altered to match
      its :attr:`point.type`, similar to changing a point's type using the UI.
      Note that FontForge's point conversion algorithm is not sophisticated
      and may not have the desired result.

   .. object:: hvcurve

      This extra flag can be used to include :data:`splineHVCurve` among the
      types that can be assigned "by geometry". Normally FontForge assigns
      :data:`splineCurve` to on-curve points with strictly horizontal or
      vertical off-curve points.

.. method:: glyph.simplify([error_bound, flags, tan_bounds, linefixup, linelenmax])

   Tries to remove excess points in the glyph if doing so will not perturb the
   curve by more than ``error-bound``. Flags is a tuple of the following strings

   .. object:: ignoreslopes

      Allow slopes to change

   .. object:: ignoreextrema

      Allow removal of extrema

   .. object:: smoothcurves

      Allow curve smoothing

   .. object:: choosehv

      Snap to horizontal or vertical

   .. object:: forcelines

      flatten bumps on lines

   .. object:: nearlyhvlines

      Make nearly horizontal/vertical lines be so

   .. object:: mergelines

      Merge adjacent lines into one

   .. object:: setstarttoextremum

      Rotate the point list so that the start point is on an extremum

   .. object:: removesingletonpoints

      If the contour contains just one point then remove it

.. method:: glyph.stroke("circular", width[, CAP, JOIN, FLAGS])
            glyph.stroke("elliptical", width, minor_width, ANGLE[, CAP, JOIN, FLAGS])
            glyph.stroke("calligraphic", width, height, angle[, FLAGS])
            glyph.stroke("polygon", contour[, FLAGS])
   (Legacy interface)
   :noindex:

.. method:: glyph.stroke("circular", width[, CAP, JOIN, ANGLE, KEYWORD])
            glyph.stroke("elliptical", width, minor_width[, ANGLE, CAP, JOIN, KEYWORD])
            glyph.stroke("calligraphic", width, height[, ANGLE, CAP, JOIN, KEYWORD])
            glyph.stroke("convex", contour[, ANGLE, CAP, JOIN, KEYWORD])
   (Current interface)

   Strokes the contours of the glyph according to the supplied parameters. See
   the :doc:`stroke </techref/stroke>` documentation for a more complete description of
   the facility and its parameters.

   A ``"circular"`` nib just has a ``width`` (the diameter), while an
   ``"elliptical"`` nib has a ``width`` (major axis) and a ``minor_width``
   (minor axis). A ``"calligraphic"`` or ``"rectangular"`` nib is similar in
   that it has a ``width`` and a ``height``. Finally a ``"convex"`` nib is one
   supplied by the user as a :class:`fontforge.contour` or :class:`fontforge.layer`.
   It must be *convex* as defined in the main stroke facility documentation.

   ``ANGLE`` is optional. It can be specified either positionally or with
   ``angle=float``. It must be a floating point number in units of radians and
   defaults to zero. The nib is rotated by this angle before stroking the path.

   ``CAP`` is optional. It can be specified either positionally or with
   ``cap=string``. It must be one of the strings "nib" (the default), "butt",
   "round", and "bevel".

   ``JOIN`` is optional. It can be specified either positionally or with
   ``join=string``. It must be one of the strings "nib" (the default), "bevel",
   "miter", and "miterclip", "round", and "arcs".

   ``KEYWORD`` Parameters:

   .. object:: removeinternal (boolean, default=False)

      When a contour is closed and clockwise, only the smaller "inside" contour
      is retained. When a contour is closed and counter-clockwise only the
      larger "outside" contour is retained.

   .. object:: removeexternal (boolean, default=False)

      When a contour is closed and clockwise, only the larger "outside" contour
      is retained. When a contour is closed and counter-clockwise only the
      smaller "inside" contour is retained.

   .. object:: extrema (boolean, default=True)

      When true, any missing extrema on the stroked paths are added.

   .. object:: simplify (boolean, default=True)

      When true, simplify is called on the path before it is returned. The
      ``error-bound`` is set to the ``accuracy`` value.

   .. object:: removeoverlap (string, default="layer")

      Specifies whether, and on what basis, remove-overlap should be run.
      "layer" corresponds to running remove-overlap on the :class:`layer` as a
      whole. "contour" corresponds to running remove-overlap on individual
      contours. "none" corresponds to not running remove-overlap. Note that
      because the stroke facility relies on remove-overlap to eliminate cusps
      and other artifacts, "none" is an unusual choice and available primarily
      for debugging purposes.

   .. object:: accuracy (float, default=0.25)

      This is a target (but not a guarantee) for the allowed error, in em-units,
      of the output relative to the input path and nib geometries. Higher values
      allow more error will typically yield contours with fewer points.

   .. object:: jlrelative (boolean, default=True)

      See below.

   .. object:: joinlimit (float, default=20)

      Specifies the maximum length of a "miter", "miterclip", or "arcs" join.
      For "miter" joins that would be longer will fall back to "bevel". With
      "miterclip" and "arcs" a longer join will be trimmed to the specified
      length. Note, however, that no join is trimmed past the "bevel line" and
      therefore lower values do not guarantee a given length.


      When ``jlrelative`` is false the value is interpreted as a length in
      em-units. Otherwise the value is interpreted as a multiple of
      "stroke-widths": the average of the spans of the nib at the incoming
      and outgoing join angles.

   .. object:: ecrelative (boolean, default=True)

      See below.

   .. object:: extendcap (float, default=0)

      When the contour being stroked is open and the ``cap`` style is "butt" or
      "round", this parameter adds area between the end of that contour and the
      cap. The length of that area will never be less than the specified value
      but may be more, depending on the geometry of the nib and the join.
      (However, it will always be exact for circular nibs.)


      When ``ecrelative`` is false the value is interpreted as a length in
      em-units. Otherwise the value is interpreted as a multiple of
      "stroke-widths": the span of the stroked path at the angle at the cap.

   .. object:: arcsclip (string, default="auto")

      When using the "arcs" join style this parameter influences the algorithm
      used to clip joins that exceed the ``joinlimit``. The value "svg2"
      specifies the standard SVG algorithm while the value "ratio" specifies an
      alternative algorithm that works better for longer and thinner nibs at
      shorter limits. The default value "auto" chooses the "ratio" algorithm
      for oblong elliptical and calligraphic nibs and
      ``jlrelative joinlimit`` < 4 and the "svg2" algorithm otherwise.

   In the legacy interface, ``FLAGS`` is an optional tuple containing zero or
   more of the strings "removeinternal", "removeexternal", and "cleanup". The
   last is interpreted as ``simplify=True``, with a default of ``False`` when a
   FLAGS tuple is present.

.. method:: glyph.transform(matrix[, flags])

   Transforms the glyph by the matrix. The optional flags argument should be a
   tuple containing any of the following strings:

   .. object:: partialRefs

      Don't transform any references in the glyph, but do transform their offsets.
      This is useful if the refered glyph will be (or has been) transformed.

   .. object:: round

      Round to int after the transformation is done.


.. method:: glyph.nltransform(xexpr, yexpr)

   xexpr and yexpr are strings specifying non-linear transformations that will
   be applied to all points in the current layer (with xexpr being applied to x
   values, and yexpr to y values, of course). The syntax for the expressions is
   explained in the :ref:`non-linear transform dialog <transform.Non-Linear>`.

.. method:: glyph.unlinkRef([ref_name])

   Unlinks the reference to the glyph named ``ref-name``. If ``ref-name`` is
   omitted, unlinks all references.

.. method:: glyph.unlinkThisGlyph()

   Unlinks all the references to the current glyph within any other glyph in
   the font.

.. method:: glyph.useRefsMetrics(ref_name[, flag])

   Finds a reference with the given name and sets the "use_my_metrics" flag on
   it (so this glyph will have the same advance width as the glyph the
   reference points to).

   If the optional flag argument is False, then the glyph will no longer have
   its metrics bound to the reference.

.. method:: glyph.validate([force])

   Validates the glyph and returns the :attr:`validation_state` of the glyph
   (except bit 0x1 will always be clear). If the glyph passed the validation
   then the return value will be 0 (not 0x1). Otherwise the return value will
   be the set of errors found. If force is specified true this will always be
   validated, if force is unspecified (or specified as false) then it will
   return the cached value if it is known, otherwise will validate it.

.. method:: glyph.draw(pen)

   Draw the glyph's outline to the `pen argument. <http://robofab.org/objects/pens.html>`_

.. method:: glyph.glyphPen([replace=False])

   Creates a new glyphPen which will draw into the current glyph. By default
   the pen will replace any existing contours and references, but setting the
   optional keyword argument, ``replace`` to false will retain the old contents.


Selection
---------

This represents a font's selection. You may index it with an encoding value (in
the encoding ISO-646-US (ASCII) the character "A" has encoding index 65), or
with a glyph's name, or with a string like ``"uXXXXX"`` where ``XXXXX``
represent the glyph's unicode codepoint in hex, or with a
:class:`fontforge.glyph` object. The value of indexing into a selection will be
either ``True`` or ``False``. ::

   >>> print(fontforge.activeFont().selection[65])
   True

This type may not be pickled.

.. class:: selection()

.. attribute:: selection.byGlyphs

   Returns another selection, just the same as this one except that its iterator
   function will return glyphs (rather than encoding slots) and will only return
   those entries for which glyphs exist.

   This is read-only.

.. method:: selection.__iter__()

   Returns an iterator for the selection which will return all selected
   encoding slots in encoding order.

.. method:: selection.all()

   Select everything.

.. method:: selection.none()

   Deselect everything.

.. method:: selection.changed()

   Select all glyphs which have changed.

.. method:: selection.invert()

   Invert the selection.

.. method:: selection.select(args)

   There may be an arbitrary number of arguments. Each argument may be either:

   * A glyph name

     Note: There need not be a glyph with this name in the font yet, but if you
     use a standard name (like "A") fontforge will still know where that glyph
     should be.
   * An integer (this will be interpreted as either an encoding index or
     (default) a unicode code point depending on the flags).
   * A fontforge glyph.
   * A tuple of flags.

     (If you wish to specify a single flag it must still be in a tuple, and you
     must append a trailing comma to the flag (so ``("more",)`` rather than
     just ``("more")`` ). FF needs the flags to be in a tuple otherwise it
     can't distinguish them from glyph names)

     .. object:: unicode

        Interpret integer arguments as unicode code points

     .. object:: encoding

        Interpret integer arguments as encoding indeces.

     .. object:: more

        Specified items should be selected

     .. object:: less

        Specified items should be deselected.

     .. object:: singletons

        Specified items should be interpreted individually and mean the obvious.

     .. object:: ranges

        Specified items should be interpreted in pairs and represent all
        encoding slots between the start and end points specified by the pair.
        So ``.select(("ranges",None),"A","Z")`` would select all the upper case
        (latin) letters.


   If the first argument is not a flag argument (or if it doesn't specify
   either "more" or "less") then the selection will be cleared. So
   ``.select("A")`` would produce a selection with only "A" selected,
   ``.select(("more",None),"A")`` would add "A" to the current selection, while
   ``.select(("less",None),"A")`` would remove "A" from the current selection.


Private
-------

This represents a font's postscript private dictionary. You may index it with
one of the standard names of things that live in the private dictionary.

This type may not be pickled.

.. class:: private

.. method:: private.__iter__()

   Returns an iterator for the dictionary which will return all entres.

.. method:: private.guess(name)

   Guess a value for this this entry in the private dictionary. If FontForge
   can't make a guess it will simply ignore the request.


Math
----

This represents a font's math constant table. Not all fonts have math tables,
and checking this field will not create the underlying object, but examining or
assigning to its members will create it.

This type may not be pickled.

.. rubric:: Members

Any of the math constant names may be used as member names.

(These names begin with capital letters, not Python's conventions but Microsoft's)

These all take (16 bit) integer values.

I do not currently provide python access to any associated device tables.


.. attribute:: math.ScriptPercentScaleDown

   Percentage scale down for script level 1

.. attribute:: math.ScriptScriptPercentScaleDown

   Percentage scale down for script level 2

.. attribute:: math.DelimitedSubFormulaMinHeight

   Minimum height at which to treat a delimited expression as a subformula

.. attribute:: math.DisplayOperatorMinHeight

   Minimum height of n-ary operators (integration, summation, etc.)

.. attribute:: math.MathLeading

   White space to be left between math formulae to ensure proper line spacing.

.. attribute:: math.AxisHeight

   Axis height of the font

.. attribute:: math.AccentBaseHeight

   Maximum (ink) height of accent base that does not require raising the accents.

.. attribute:: math.FlattenedAccentBaseHeight

   Maximum (ink) height of accent base that does not require flattening the accents.

.. attribute:: math.SubscriptShiftDown

   The standard shift down applied to subscript elements. Positive for
   moving downward.

.. attribute:: math.SubscriptTopMax

   Maximum height of the (ink) top of subscripts that does not require moving
   subscripts further down.

.. attribute:: math.SubscriptBaselineDropMin

   Maximum allowed drop of the baseline of subscripts relative to the bottom of
   the base. Used for bases that are treated as a box or extended shape.
   Positive for subscript baseline dropped below base bottom.

.. attribute:: math.SuperscriptShiftUp

   Standard shift up applied to superscript elements.

.. attribute:: math.SuperscriptShiftUpCramped

   Standard shift of superscript relative to base in cramped mode.

.. attribute:: math.SuperscriptBottomMin

   Minimum allowed height of the bottom of superscripts that does not require
   moving them further up.

.. attribute:: math.SuperscriptBaselineDropMax

   Maximum allowed drop of the baseline of superscripts relative to the top of
   the base. Used for bases that are treated as a box or extended shape.
   Positive for superscript baseline below base top.

.. attribute:: math.SubSuperscriptGapMin

   Minimum gap between the superscript and subscript ink.

.. attribute:: math.SuperscriptBottomMaxWithSubscript

   The maximum level to which the (ink) bottom of superscript can be pushed to
   increase the gap between superscript and subscript, before subscript starts
   being moved down.

.. attribute:: math.SpaceAfterScript

   Extra white space to be added after each sub/superscript.

.. attribute:: math.UpperLimitGapMin

   Minimum gap between the bottom of the upper limit, and the top of the base
   operator.

.. attribute:: math.UpperLimitBaselineRiseMin

   Minimum distance between the baseline of an upper limit and the bottom of
   the base operator.

.. attribute:: math.LowerLimitGapMin

   Minimum gap between (ink) top of the lower limit, and (ink) bottom of the
   base operator.

.. attribute:: math.LowerLimitBaselineDropMin

   Minimum distance between the baseline of the lower limit and bottom of the
   base operator.

.. attribute:: math.StackTopShiftUp

   Standard shift up applied to the top element of a stack.

.. attribute:: math.StackTopDisplayStyleShiftUp

   Standard shift up applied to the top element of a stack in display style.

.. attribute:: math.StackBottomShiftDown

   Standard shift down applied to the bottom element of a stack. Positive
   values indicate downward motion.

.. attribute:: math.StackBottomDisplayStyleShiftDown

   Standard shift down applied to the bottom element of a stack in display
   style. Positive values indicate downward motion.

.. attribute:: math.StackGapMin

   Minimum gap between bottom of the top element of a stack, and the top of
   the bottom element.

.. attribute:: math.StackDisplayStyleGapMin

   Minimum gap between bottom of the top element of a stack and the top of the
   bottom element in display style.

.. attribute:: math.StretchStackTopShiftUp

   Standard shift up applied to the top element of the stretch stack.

.. attribute:: math.StretchStackBottomShiftDown

   Standard shift down applied to the bottom element of the stretch stack.
   Positive values indicate
   downward motion.

.. attribute:: math.StretchStackGapAboveMin

   Minimum gap between the ink of the stretched element and the ink bottom of
   the element above.

.. attribute:: math.StretchStackGapBelowMin

   Minimum gap between the ink of the stretched element and the ink top of
   the element below.

.. attribute:: math.FractionNumeratorShiftUp

   Standard shift up applied to the numerator.

.. attribute:: math.FractionNumeratorDisplayStyleShiftUp

   Standard shift up applied to the numerator in display style.

.. attribute:: math.FractionDenominatorShiftDown

   Standard shift down applied to the denominator. Positive values indicate
   downward motion.

.. attribute:: math.FractionDenominatorDisplayStyleShiftDown

   Standard shift down applied to the denominator in display style. Positive
   values indicate downward motion.

.. attribute:: math.FractionNumeratorGapMin

   Minimum tolerated gap between the ink bottom of the numerator and the ink of
   the fraction bar.

.. attribute:: math.FractionNumeratorDisplayStyleGapMin

   Minimum tolerated gap between the ink bottom of the numerator and the ink of
   the fraction bar in display style.

.. attribute:: math.FractionRuleThickness

   Thickness of the fraction bar.

.. attribute:: math.FractionDenominatorGapMin

   Minimum tolerated gap between the ink top of the denominator and the ink of
   the fraction bar.

.. attribute:: math.FractionDenominatorDisplayStyleGapMin

   Minimum tolerated gap between the ink top of the denominator and the ink of
   the fraction bar in display style.

.. attribute:: math.SkewedFractionHorizontalGap

   Horizontal distance between the top and bottom elements of a skewed fraction.

.. attribute:: math.SkewedFractionVerticalGap

   Vertical distance between the ink of the top and bottom elements of a skewed
   fraction.

.. attribute:: math.OverbarVerticalGap

   Distance between the overbar and the ink top of the base.

.. attribute:: math.OverbarRuleThickness

   Thickness of the overbar.

.. attribute:: math.OverbarExtraAscender

   Extra white space reserved above the overbar.

.. attribute:: math.UnderbarVerticalGap

   Distance between underbar and the (ink) bottom of the base.

.. attribute:: math.UnderbarRuleThickness

   Thickness of the underbar.

.. attribute:: math.UnderbarExtraDescender

   Extra white space reserved below the underbar.

.. attribute:: math.RadicalVerticalGap

   Space between the ink to of the expression and the bar over it.

.. attribute:: math.RadicalDisplayStyleVerticalGap

   Space between the ink top of the expression and the bar over it in display
   style.

.. attribute:: math.RadicalRuleThickness

   Thickness of the radical rule in designed or constructed radical signs.

.. attribute:: math.RadicalExtraAscender

   Extra white space reserved above the radical.

.. attribute:: math.RadicalKernBeforeDegree

   Extra horizontal kern before the degree of a radical if such be present.

.. attribute:: math.RadicalKernAfterDegree

   Negative horizontal kern after the degree of a radical if such be present.

.. attribute:: math.RadicalDegreeBottomRaisePercent

   Height of the bottom of the radical degree, if such be present, in
   proportion to the ascender of the radical sign.

.. attribute:: math.MinConnectorOverlap

   Minimum overlap of connecting glyphs during glyph construction.

.. method:: math.exists()

   Returns whether the font currently has an underlying math table associated
   with it. Note that examining or assigning to one of the members will create
   such a table.

.. method:: math.clear()

   Removes any underlying math table from the font.

Font
----

The font type refers to a fontforge :class:`font` object. It generally contains
a list of :class:`glyphs <fontforge.glyph>`, an encoding to order those glyphs,
a fontname, a list of GPOS/GSUB lookups and many other things.

This type may not be pickled.

.. class:: font()

   Creates a new font.

.. attribute:: font.activeLayer

   Returns currently active layer in the font (as an integer). May be set to an
   integer or a layer name to change the active layer.

.. attribute:: font.ascent

   The font's ascent

.. attribute:: font.bitmapSizes

   A tuple with an entry for each bitmap strike attached to the font. Each
   strike is identified by pixelsize (if the strike is a grey scale font it
   will be indicated by ``(bitmap-depth<<16)|pixelsize``.

   When setting this value pass in a tuple of the same format. Any existing
   strike not specified in the tuple will be removed. Any new sizes will be
   created (but not rasterized -- use :meth:`font.regenBitmaps()` for that).

.. attribute:: font.capHeight

   (readonly) Computes the Cap Height (the height of capital letters such as
   "E"). A negative number indicates the value could not be computed (the font
   might have no capital letters because it was lower case only, or didn't
   include glyphs for a script with capital letters).

.. attribute:: font.changed

   Bit indicating whether the font has been modified. This is (should be)
   maintained automatically, but you may set it if you wish.

.. attribute:: font.cidcopyright

   Copyright message of the cid-keyed font as a whole (ie. not the current subfont).

.. attribute:: font.cidfamilyname

   Family name of the cid-keyed font as a whole (ie. not the current subfont).

.. attribute:: font.cidfontname

   Font name of the cid-keyed font as a whole (ie. not the current subfont).

.. attribute:: font.cidfullname

   Full name of the cid-keyed font as a whole (ie. not the current subfont).

.. attribute:: font.cidordering


.. attribute:: font.cidregistry


.. attribute:: font.cidsubfont

   Returns the number index of the current subfont in the cid-keyed font (or -1
   if this is not a cid-keyed font).

   May be set to an index (an integer) or a subfont fontname (a string) to
   change the current subfont. (To find the name of the current subfont,
   simply use .fontname).

.. attribute:: font.cidsubfontcnt

   Returns the number of subfonts in this cid-keyed font (or 0 if it is not a
   cid-keyed font)

.. attribute:: font.cidsubfontnames

   Returns a tuple of the subfont names in this cid-keyed font (or None if it
   is not a cid-keyed font)

.. attribute:: font.cidsupplement


.. attribute:: font.cidversion


.. attribute:: font.cidweight

   Weight of the cid-keyed font as a whole

.. attribute:: font.comment

   A comment associated with the font. Can be anything.

.. attribute:: font.copyright

   PostScript copyright notice

.. attribute:: font.cvt

   Returns a sequence object containing the font's cvt table. Changes made
   to this object will be made to the font (this is a reference not a copy).

   The object has one additional method ``cvt.find(value[,low,high])`` which
   finds the index of value in the cvt table (or -1 if not found). If low and
   high are specified then the index will be between ``[low,high)``.

.. attribute:: font.default_base_filename

   The default base for the filename when generating a font

.. attribute:: font.descent

   The font's descent

.. attribute:: font.design_size

   Size (in pica points) for which this font was designed.

.. attribute:: font.em

   The em size of the font. Setting this will scale the entire font to the
   new size.

.. attribute:: font.encoding

   The name of the current encoding. Setting it will change the encoding used
   for indexing. To compact the encoding, first set it to your desired encoding
   (e.g. ``UnicodeBMP``), then set it to ``compacted``.

.. attribute:: font.familyname

   PostScript font family name

.. attribute:: font.fondname

   Mac fond name

.. attribute:: font.fontlog

   A comment associated with the font. Can be anything.

.. attribute:: font.fontname

   PostScript font name

   Note that in a CID keyed font this will be the name of the current subfont.
   Use cidfontname for the name of the font as a whole.

.. attribute:: font.fullname

   PostScript font name

.. attribute:: font.gasp

   Returns a tuple of all gasp table entries. Each item in the tuple is itself
   a tuple composed of a ppem (an integer) and a tuple of flags. The flags are
   chosen from:

   * ``gridfit``
   * ``antialias``
   * ``symmetric-smoothing``
   * ``gridfit+smoothing``

.. attribute:: font.gasp_version

   The version of the 'gasp' table. Currently this may be 0 or 1.

.. attribute:: font.gpos_lookups

   Returns a tuple of all positioning lookup names in the font.
   This member cannot be set.

.. attribute:: font.gsub_lookups

   Returns a tuple of all substitution lookup names in the font.
   This member cannot be set.

.. attribute:: font.guide

   A copy of the font's guide layer

.. attribute:: font.hasvmetrics


.. attribute:: font.head_optimized_for_cleartype


.. attribute:: font.hhea_ascent


.. attribute:: font.hhea_ascent_add


.. attribute:: font.hhea_descent


.. attribute:: font.hhea_descent_add


.. attribute:: font.hhea_linegap


.. attribute:: font.horizontalBaseline

   Returns a tuple of tuples containing the horizontal baseline information in
   the font (the 'BASE' table). If there is no information ``None`` will be
   returned, otherwise the format of the tuple is: ::

      ((tuple of baseline tags used), (tuple of script information))

   The ``(tuple of baseline tags used)`` is simply a tuple of 4 letter strings
   as ``("hang", "ideo", "romn")`` these are standard baseline tag names as
   defined in the opentype spec. The number of entries here, and their order is
   important as there will be subsequent tuples (in the script tuple) which use
   the same ordering.

   The ``(tuple of script information)`` is again a tuple of
   ``script information`` tuples.

   A ``script information`` tuple looks like ::

      (script-tag,default-baseline-tag, (tuple of baseline positions), (tuple of language extents))

   If there are no baseline tags defined (an empty tuple), then the
   ``default-baseline-tag`` and the ``(tuple of baseline positions)`` will be
   ``None``. Otherwise both tags will be 4 character strings, and the ``(tuple
   of baseline positions)`` will be a tuple of numbers (in the same order as the
   ``(tuple of baseline tags used)`` above) specifying the relative positions
   of each baseline for this script.

   A ``(tuple of language extents)`` is a tuple of ``language extent`` tuples.

   A ``language extent`` tuple is ::

      (language-tag,min-extent,max-extent, (tuple of feature extents))

   ``language-tag`` is a 4 letter string specifying an opentype language,
   ``min``/``max-extent`` are numbers specifying how far above and below the
   baseline characters go in this script/language.

   A ``(tuple of feature extents>`` is a tuple of ``feature extent`` tuples.

   A ``feature extent`` tuple is ::

      (feature-tag,min-extent,max-extent)

   ``feature-tag`` is a 4 letter string specifying an opentype (GPOS or GSUB)
   feature tag, ``min``/``max-extent`` are numbers specifying how far above and
   below the baseline characters go in this script/language with the
   feature applied.

   **Example:**

   ::

      (("hang","ideo","romn"),
        (("cyrl","romn",(1405,-288,0),()),
         ("grek","romn",(1405,-288,0),()),
         ("latn","romn",(1405,-288,0),
           (("dflt",-576,1913,
             (("NoAc",-576,1482),
              ("ENG ",-576,1482))
           ),
         )
        )
       )
      )

   (Note: The comma after the ``dflt`` tuple puts it into a one-element tuple.)

.. attribute:: font.is_cid

   Indicates whether the font is a cid-keyed font or not. (Read-only)

.. attribute:: font.is_quadratic

   Deprecated. Whether the contours should be interpretted as a set of quadratic
   or cubic splines. Setting this value has the side effect of converting the
   entire font into the other format

   Now each layer may have its own setting for this value, which should be set
   on the font's :attr:`font.layers`.

.. attribute:: font.isnew

   A flag indicating that this is a new font

.. attribute:: font.italicangle


.. attribute:: font.macstyle

   .. object:: Bit 0

      Bold (if set to 1)

   .. object:: Bit 1

      Italic (if set to 1)

   .. object:: Bit 2

      Underline (if set to 1)

   .. object:: Bit 3

      Outline (if set to 1)

   .. object:: Bit 4

      Shadow (if set to 1)

   .. object:: Bit 5

      Condensed (if set to 1)

   .. object:: Bit 6

      Extended (if set to 1)

   .. object:: Bits 7-15

      Reserved (set to 0).

   (`source <https://docs.microsoft.com/en-us/typography/opentype/spec/head>`_)

.. attribute:: font.layer_cnt

   The number of layers in the font. (Read only. Can change using ``add``
   and ``del`` operations on the :attr:`font.layers` array)

.. attribute:: font.layers

   Returns a dictionary like object with information on the layers of the
   font -- a name and a boolean indicating whether the layer is quadratic or not.

   You may remove a layer with ::

      del font.layers["unneeded layer"]

   You may add a new layer with ::

      font.layers.add("layer-name",is_quadratic[, is_background])

   You may change a layer's name with ::

      font.layers["layer"].name = "new-name"

   You may change the type of splines in a layer with ::

      font.layers["layer"].is_quadratic = True

   You may change whether it is a background layer by ::

      font.layers["layer"].is_background = True

   Note: The layers that live in the font are different from layers that live
   in a glyph. These objects do not have the Layer type documented earlier.

.. attribute:: font.loadState

   A bitmask indicating non-fatal errors found when loading the font. (readonly)

   .. object:: 0x01

      Bad PostScript entry in 'name' table

   .. object:: 0x02

      Bad 'glyf' or 'loca' table

   .. object:: 0x04

      Bad 'CFF ' table

   .. object:: 0x08

      Bad 'hhea', 'hmtx', 'vhea' or 'vmtx' table

   .. object:: 0x10

      Bad 'cmap' table

   .. object:: 0x20

      Bad 'EBLC', 'bloc', 'EBDT' or 'bdat' (embedded bitmap) table

   .. object:: 0x40

      Bad Apple GX advanced typography table

   .. object:: 0x80

      Bad OpenType advanced typography table (GPOS, GSUB, GDEF, BASE)

   .. object:: 0x100

      Bad OS/2 version number

      Windows will reject all fonts with a OS/2 version number of 0 and will
      reject OT-CFF fonts with a version number of 1


.. attribute:: font.maxp_FDEFs

   The number of function definitions used by the tt program

.. attribute:: font.maxp_IDEFs

   The number of instruction definitions used by the tt program

.. attribute:: font.maxp_maxStackDepth

   The maximum stack depth used by the tt program

.. attribute:: font.maxp_storageCnt

   The number of storage locations used by the tt program

.. attribute:: font.maxp_twilightPtCnt

   The number of points in the twilight zone of the tt program

.. attribute:: font.maxp_zones

   The number of zones used in the tt program

.. attribute:: font.multilayer


.. attribute:: font.onlybitmaps

   A flag indicating that this font only contains bitmaps. No outlines.

.. attribute:: font.os2_codepages

   A 2 element tuple containing the OS/2 Codepages field

.. attribute:: font.os2_family_class


.. attribute:: font.os2_fstype


.. attribute:: font.os2_panose


.. attribute:: font.os2_strikeypos


.. attribute:: font.os2_strikeysize


.. attribute:: font.os2_subxoff


.. attribute:: font.os2_subxsize


.. attribute:: font.os2_subyoff


.. attribute:: font.os2_subysize


.. attribute:: font.os2_supxoff


.. attribute:: font.os2_supxsize


.. attribute:: font.os2_supyoff


.. attribute:: font.os2_supysize


.. attribute:: font.os2_typoascent


.. attribute:: font.os2_typoascent_add


.. attribute:: font.os2_typodescent


.. attribute:: font.os2_typodescent_add


.. attribute:: font.os2_typolinegap


.. attribute:: font.os2_use_typo_metrics


.. attribute:: font.os2_unicoderanges

   A 4 element tuple containing the OS/2 Unicode Ranges field

.. attribute:: font.os2_vendor


.. attribute:: font.os2_version


.. attribute:: font.os2_weight


.. attribute:: font.os2_weight_width_slope_only


.. attribute:: font.os2_width


.. attribute:: font.os2_winascent


.. attribute:: font.os2_winascent_add


.. attribute:: font.os2_windescent


.. attribute:: font.os2_windescent_add


.. attribute:: font.path

   (readonly) Returns a string containing the name of the file from which the
   font was originally read (in this session), or if this is a new font, returns
   a made up filename in the current directory named something like
   "Untitled1.sfd". See also :attr:`font.sfd_path`.

.. attribute:: font.persistent

   Whatever you want -- though I recommend you store a dict here (these data
   will be saved as a pickled object in the sfd file. It is your job to ensure
   that whatever you put here can be pickled)

   If you do store a dict then the following entries will be treated specially:

   .. object:: initScriptString

      If present, and if this is a string, then each time the font is loaded
      from an sfd file, this string will be passed to the python interpreter.

      .. note::

         This is a string, not a function.
         Function code cannot be pickled. Since it is a string it will receive
         no arguments, but the current font will be available in the activeFont
         method of the fontforge module.

      This string will be interpreted before the loadFontHook of the
      :data:`fontforge.hooks` dictionary.

      One possible behavior for this string is to define function hooks to
      be stored in the temporary dict described below.


.. attribute:: font.math

   Returns a :class:`math` object which provides information on the font's
   underlying math constant table. There is only one of these per font.

.. attribute:: font.private

   Returns a :class:`private` dictionary-like object representing the
   PostScript private dictionary for the font. Changing entries in this object
   will change them in the font. (It's a reference, not a copy).

   There is an iterator associated with this entry.

.. attribute:: font.privateState

   Checks the (PostScript) Private dictionary and returns a bitmask of some
   common errors.

   .. object:: 0x000001

      Odd number of elements in either the BlueValues or OtherBlues array.

   .. object:: 0x000002

      Elements in either the BlueValues or OtherBlues are disordered.

   .. object:: 0x000004

      Too many elements in either the BlueValues or OtherBlues array.

   .. object:: 0x000008

      Elements in either the BlueValues or OtherBlues array are too close
      (must be at least ``2*BlueFuzz +1`` apart).

   .. object:: 0x000010

      Elements in either the BlueValues or OtherBlues array are not integers.

   .. object:: 0x000020

      Alignment zone height in either the BlueValues or OtherBlues array is too
      big for the value of BlueScale.

   .. object:: 0x000100

      Odd number of elements in either the FamilyBlues or FamilyOtherBlues array.

   .. object:: 0x000200

      Elements in either the FamilyBlues or FamilyOtherBlues are disordered.

   .. object:: 0x000400

      Too many elements in either the FamilyBlues or FamilyOtherBlues array.

   .. object:: 0x000800

      Elements in either the FamilyBlues or FamilyOtherBlues array are too
      close (must be at least ``2*BlueFuzz +1`` apart).

   .. object:: 0x001000

      Elements in either the FamilyBlues or FamilyOtherBlues array are not
      integers.

   .. object:: 0x002000

      Alignment zone height in either the FamilyBlues or FamilyOtherBlues array
      is too big for the value of BlueScale.

   .. object:: 0x010000

      Missing BlueValues entry.

   .. object:: 0x020000

      Bad BlueFuzz entry.

   .. object:: 0x040000

      Bad BlueScale entry.

   .. object:: 0x080000

      Bad StdHW entry.

   .. object:: 0x100000

      Bad StdVW entry.

   .. object:: 0x200000

      Bad StemSnapH entry.

   .. object:: 0x400000

      Bad StemSnapV entry.

   .. object:: 0x800000

      StemSnapH does not include StdHW.

   .. object:: 0x1000000

      StemSnapV does not include StdVW.

   .. object:: 0x2000000

      Bad BlueShift entry.


.. attribute:: font.selection

   Returns a reference to a :class:`array-like object representing the font's
   selection <selection>`. There is one entry for each encoding slot (there may
   not be a glyph attached to every encoding slot). You may set this with a
   tuple of integers (or boolean values). There should not be more entries in
   the tuple than there are encoding slots in the current encoding. A ``True``
   or non-0 value means the slot is selected.

.. attribute:: font.sfd_path

   (readonly) Returns a string (or None) containing the name of the sfd file
   associated with this font. Sometimes this will be the same as :attr:`font.path`.

.. attribute:: font.sfnt_names

   The strings in the sfnt 'name' table. A tuple of all MS names. Each name is
   itself a tuple of strings ``(language,strid,string)``. Language may be
   either the (english) name of the language/locale, or the number representing
   that language in Microsoft's specification. Strid may be one of the
   (English) string names ``(Copyright, Family, SubFamily, etc.)`` or the
   numeric value of that item. The string itself is in UTF-8.

   Mac names will be automagically created from MS names

.. attribute:: font.sfntRevision

   The font revision field stored in the ``'head'`` table of an sfnt. This
   is documented to be a fixed 16.16 number (that is a 32 bit number with the
   binary point between bits 15 and 16).

   The field may be unset (in which case when the font is generated, FontForge
   will guess a default value from one of the version strings).

   The value returned with be ``None`` if the field is unset or a double.

   You may set it to ``None`` which "unsets" it, or to a double value, or to an
   integer. The integer will be treated as a 32 bit integer and right shifted
   by 16 to get a 16.16 value).

.. attribute:: font.size_feature

   The OpenType 'size' feature has two formats. It may either represent the
   design size of the font (and nothing else) or the design size, and range
   (top and bottom point sizes for which this design works), a style id (used
   to represent this design size throughout the font family) and a set of
   language/string pairs used to represent this design size in the menu.

   If no size information is specified in the font FontForge will return None.

   If only the design size is specified, FontForge will return a tuple
   containing a single element: the point size for which the font was designed.
   (This is returned as a real number -- the field can represent tenths of a point).

   Otherwise FontForge returns a tuple containing five elements, the design
   size, the bottom of the design range, the top, the style id and a tuple of
   tuples. Each sub-tuple is a language/string pair. Language may be either
   the (english) name of the language/locale, or The string itself is in UTF-8.

.. attribute:: font.strokedfont

   is this a stroked font?

.. attribute:: font.strokewidth

   the stroke width of a stroked font

.. attribute:: font.temporary

   Whatever you want -- though I recommend you store a dict here (these data
   will be lost once the font is closed)

   If you do store a dict then the following entries will be treated specially:

   .. object:: generateFontPreHook

      If present, and if this is a function it will be called just before a
      font is generated. It will be called with the font and the filename to
      which the font will be written.

   .. object:: generateFontPostHook

      If present, and if this is a function it will be called just after a font
      is generated. It will be called with the font and the filename to which
      the font will be written.

.. attribute:: font.texparameters

   Returns a tuple of :ref:`TeX font parameters <fontinfo.TeX>`. TeX font type
   followed by 22 parameters. Font type is one of:

     * ``text``
     * ``mathsym``
     * ``mathext``
     * ``unset``


   In case of ``unset`` default values for font parameters will be returned.

.. attribute:: font.uniqueid


.. attribute:: font.upos

   underline position

.. attribute:: font.userdata

   .. warning:: Deprecated name for :attr:`font.temporary`

.. attribute:: font.uwidth

   underline width

.. attribute:: font.version

   PostScript font version string

.. attribute:: font.verticalBaseline

   Same format as :attr:`font.horizontalBaseline`.

.. attribute:: font.vertical_origin

   .. warning:: Deprecated

.. attribute:: font.vhea_linegap


.. attribute:: font.weight

   PostScript font weight string

.. attribute:: font.woffMajor

   The major version number of a woff file, an integer.

   The field may be unset (in which case when the font is generated, FontForge
   will guess a default value from one of the version strings).

   The value returned with be ``None`` if the field is unset or an integer.

   You may set it to ``None`` which "unsets" it, or to an integer.

.. attribute:: font.woffMinor

   The minor version number of a woff file, an integer.

   The field may be unset (in which case when the font is generated, FontForge
   will guess a default value from one of the version strings).

   The value returned with be ``None`` if the field is unset or an integer.

   You may set it to ``None`` which "unsets" it, or to an integer.

.. attribute:: font.woffMetadata

   Any metadata associated with a woff file. This is a utf8 string containing
   unparsed xml.

.. attribute:: font.xHeight

   (readonly) Computes the X Height (the height of lower case letters such as
   "x"). A negative number indicates the value could not be computed (the font
   might have no lower case letters because it was upper case only, or didn't
   include glyphs for a script with lower case letters).


.. method:: font.__iter__()

   Returns an iterator for the font which will run through the font, in gid
   order, returning glyph names

.. method:: font.__contains__()

   Returns whether the font contains a glyph with the given name.

.. method:: font.__len__()

   The number of glyph slots in the current encoding

.. method:: font.__getitem__(key)

   If ``key`` is an integer, then returns the glyph at that encoding. If a
   string then returns the glyph with that name. May not be assigned to.

.. method:: font.addAnchorClass(lookup_subtable_name, new_anchor_class_name)

   Adds an anchor class to the specified (anchor) subtable.

.. method:: font.addKerningClass(lookup_name, new_subtable_name, first_classes, second_classes, offsets[, after])
            font.addKerningClass(lookup_name, new_subtable_name, separation, first_classes, second_classes[, onlyCloser, autokern, after])
            font.addKerningClass(lookup_name, new_subtable_name, separation, class_distance, first_glyph_list, second_glyph_list, [, onlyCloser, autokern, after])
            font.addKerningClass(lookup_name, new_subtable_name, separation, class_distance, [, onlyCloser, autokern, after])

   Creates a new subtable and a new kerning class in the named lookup. The
   classes arguments are tuples of tuples of glyph names (each sub-tuble of
   glyph names is a kerning class). The offsets argument is a tuple of kerning
   offsets. There must be as many entries as ::

      len(first-class)*len(second-class)

   The optional after argument is used to specify the order of the subtable
   within the lookup.

   The second format will cause FontForge to auto kern the subtable. The
   separation argument specifies the desired optical distance between any two
   glyphs (if this is specified as 0 then the kerning class will be designed so
   glyphs just touch each other). Again the user specifies two sets of
   predefined classes. If the (optional) ``onlyCloser`` flag is set true then
   only negative kerning values will be inserted into the table.

   In the third format the user merely specifies two lists of glyphs to be
   used, fontforge will look for similarities among among the glyphs and guess
   at classes. The class-distance argument to determine how precise the classes
   should match (1 is very tight matching, 20 is rather loose).

   In the last format the font's selection will be used to specify the list of
   glyphs to be examined (and the same list will be used for both the left and
   right glyphs -- but fontforge will probably find different classes).

.. method:: font.addLookup(new_lookup_name, type, flags, feature_script_lang_tuple[, after_lookup_name)

   Creates a new lookup with the given name, type and flags. It will tag it
   with any indicated features. The type of one of

   * ``gsub_single``
   * ``gsub_multiple``
   * ``gsub_alternate``
   * ``gsub_ligature``
   * ``gsub_context``
   * ``gsub_contextchain``
   * ``gsub_revesechain``
   * ``morx_indic``
   * ``morx_context``
   * ``morx_insert``
   * ``gpos_single``
   * ``gpos_pair``
   * ``gpos_cursive``
   * ``gpos_mark2base``
   * ``gpos_mark2ligature``
   * ``gpos_mark2mark``
   * ``gpos_context``
   * ``gpos_contextchain``
   * ``kern_statemachine``

   The flags argument is a tuple of strings, or ``None``. At most one of these
   strings may be the name of a mark class. The others are:

   * ``right_to_left``
   * ``ignore_bases``
   * ``ignore_ligatures``
   * ``ignore_marks``

   A feature-script-lang tuple is a tuple with one entry for each feature
   (there may be no entries if there are no features). Each entry is itself a
   two element tuple, the first entry is a string containing a 4 letter feature
   tag, and the second entry is another tuple (potentially empty) with an entry
   for each script for which the feature is active. Each entry here is itself a
   two element tuple. The first element is a 4 letter script tag and the second
   is a tuple of languages. Each entry in the language tuple is a four letter
   language. Example: ``(("liga",(("latn",("dflt")),)),)``

   The optional final argument allows you to specify the ordering of the lookup.
   If not specified the lookup will be come the first lookup in its table.

.. method:: font.addLookupSubtable(lookup_name, new_subtable_name[, after_subtable_name])

   Creates a new subtable within the specified lookup. The lookup name should
   be a string specifying an existing lookup. The subtable name should also be
   a string and should not match any currently existing subtable in the lookup.
   The optional final argument allows you to specify the ordering within the
   lookup. If not specified this subtable will be first in the lookup.

   If you want to create a subtable in a contextual lookup, then use
   :meth:`font.addContextualSubtable`. If you want to create a kerning class
   subtable, then use :meth:`font.addKerningClass`.

.. method:: font.addContextualSubtable(lookup_name, new_subtable_name, type, rule[, afterSubtable=]  [, bclasses=]  [, mclasses=]  [, fclasses=] [, bclassnames=]  [, mclassnames=]  [, fclassnames=])

   Creates a new subtable within the specified contextual lookup (contextual,
   contextual chaining, or reverse contextual chaining). The lookup name should
   be a string specifying an existing lookup. The subtable name should also be
   a string and should not match any currently existing subtable in the lookup.

   The ``type`` should be one of the strings "glyph", "class", "coverage" or
   "reversecoverage". The ``rule`` should be a string specifying a string to
   match and a set of lookups to apply once the match has been made. (See
   below for more details).

   The remaining arguments are optional, keyword arguments.

   * ``afterSubtable=``, if present this should be followed by a string, the
     name of a subtable after which this one is to be placed in the lookup. If
     not specified this subtable will be first in the lookup.
   * ``bclasses=, fclasses=, mclasses=`` these three arguments specify sets of
     glyph classes for when ``type="class"``. They should be a tuple of
     thingies where each thingy is either a string containing a list of space
     separated glyph names, or another tuple containing a set of strings, each
     a glyph name. Note that the first class is magic and should usually be
     left as a null string.
   * ``bclassnames=, fclassnames=, mclassnames=`` These provide names for the
     glyph classes described above. These names are optional, but can be
     convenient. These are tuples of strings. There should be the same number
     of entries in ``bclassnames`` as there are in ``bclasses``.

   .. object:: When type="glyph"

      The rule should look something like: ::

         glyph-name1 glyph-name2 | glyph-name3 @<lookup-name> | glyph-name4

      The ``|`` s divide between backtrack, match and lookahead sections. So
      this example would match it the current glyph were named ``glyph-name3``
      and it were preceded by ``glyph-name2`` and that by ``glyph-name1`` and
      followed by ``glyph-name4``. If the match were successful then the lookup
      named ``lookup-name`` would be applied. The ``@<>`` are litteral
      characters and should be present in the rule.

      If the invoked lookup is a ligature lookup then it should be invoked
      after the first glyph that forms the lookup (rather than the last) and
      all glyphs that might make up the lookup should be in the match section.
      So... ::

        e | f @<ff-lig> f l | o

      would only apply the ``ff-lig`` lookup if the ``ffl`` were preceeded by
      ``e`` and followed by ``o``.

   .. object:: When type="class"

      The rule should look something like: ::

         class-name1 class-name2 | class-name3 @<lookup-name> | class-name4

      Very similar to the case of glyphs, except that instead of glyph names
      we have class names here. It is possible to have different sets of class
      names in the three different sections (backtrack, match and lookahead).
      If you don't specify any class names then you must use numbers instead,
      each number refering to the class at that position in the tuple (the first
      class will be class 0, the second class 1, and so on).

   .. object:: When type="coverage"

      The rule should look something like: ::

         [g1 g2] [g3 g4] | [g5 g6 g7] @<lookup-name> | [g8 g9]

      Each entry within brackets, ``[]``, represents a coverage table and
      should be a list of glyph names. The brackets are specified literally.

   .. object:: When type="reversecoverage"

      The rule should look something like: ::

         [g1 g2] [g3 g4] | [g5 g6 g7] => [rg1 rg2 rg3] | [g8 g9]

      Very similar to normal coverage tables except that instead of specifying
      a lookup there are replacement glyphs inline. There must be the same
      number of replacement glyphs (``rg1, rg2,rg3``) as match glyphs
      (``g5, g6, g7``) and there may be only one coverage table in the match
      section.

   .. warning::

      This format has some limitations, if there are multiple lookups they
      will be applied in textual order (First lookup in the string is the first
      one applied). This limitation is also present in Adobe's feature files so
      I hope it shan't be a severe limitation.

.. method:: font.addSmallCaps(scheight=None, capheight=None, lcstem=None, ucstem=None, symbols=None, letter_extension=None, symbol_extension=None, stem_height_factor=None)

   This function uses keyword parameters. None are required, if omitted a
   default value will be used (generally found by analyzing the font).

   For each selected letter, this function will create a corresponding small
   caps glyph. If you set the ``symbol`` keyword to ``True`` it will also
   create small caps variants of digits and symbols.

   The outlines of the new glyph will be based on the outlines of the
   upper-case variant of the letter. These will then be scaled so that a glyph
   which was ``capheight`` high will now be ``scheight`` high, and so that
   stems which were ``ucstem`` wide will be ``lcstem`` wide. Normally the ratio
   of stem heights is the same as the ratio of stem widths, but you may change
   that with ``stem_height_factor``.

   When it creates a new glyph it will name that glyph by appending ".sc" to
   the original lower case letter name (so "a" would become "a.sc") you may
   change the extension used with ``letter_extension``. Similary symbols and
   digits will use the extension "taboldstyle", but you may change that with
   ``symbol_extension``.

   When it creates a glyph it also creates two lookups one bound to the feature
   "c2sc" and the other to "smcp". A mapping from the lower case letter to the
   small caps letter will be provided under "smcp", while a mapping from the
   upper case to the small caps under "c2sc". Symbols will have both lookup
   maps attached to the original glyph.

.. method:: font.alterKerningClass(subtable_name, first_classes, second_classes, offsets)

   Changes the kerning class in the named subtable. The classes arguments are
   tuples of tuples of glyph names (each sub-tuble of glyph names is a kerning
   class). The offsets argument is a tuple of kerning offsets. There must be as
   many entries as ``len(first-class)*len(second-class)``. The optional after
   argument is used to specify the order of the subtable within the lookup.

.. method:: font.autoKern(subtable_name, separation[, minKern=, onlyCloser=, touch=])
            font.autoKern(subtable_name, separation, glyph_list1, glyph_list2[, minKern=, onlyCloser=, touch=])

   The named subtable must be a kerning pair subtable that already exists.

   This command will automatically generate kerning pairs for the named
   subtable. If no glyph lists are specified it will look at all pairs of the
   glyphs in the selection; if glyph lists are specified then it will look at
   all pairs that can be made with one glyph from the first list and the second
   from the second list.

   It will attempt to guess a good kerning value between the two glyphs -- a
   value which will make the optical separation between the two appear to be
   ``separation`` em-units. If ``minkern`` is specified then and the (absolute
   value of the) kerning correction is less than this number then no kerning
   pair will be generated. If ``onlyCloser`` is set true then only negative
   kerning offsets will be generated (only thing which move two glyphs closer
   together). If touch is set to 1 then the kerning offset will not be based on
   optical distance but on the closest approach between two the two glyphs.

.. method:: font.appendSFNTName(language, strid, string)

   Adds a new (or replaces an old) string in the sfnt 'name' table. Language
   may be either the English name of the language/locale as a string, or the
   number representing that language in MicroSoft's specification. Strid may be
   one of the (english) string names (Copyright, Family, SubFamily, etc.) or
   the numeric value of that item. The string itself is in UTF-8.

.. method:: font.buildOrReplaceAALTFeatures()

   Removes any existing AALT features (and any lookups solely controled by such
   features) and creates new ones containing all possible single and alternate
   substutions available for each glyph.

.. method:: font.cidConvertByCMap(cmap_filename)

   Converts a normal font into a CID-keyed font with one subfont using

   the CMAP to determine the mapping.

.. method:: font.cidConvertTo(registry, ordering, supplement)

   Converts a normal font into a CID-keyed font with one subfont.

.. method:: font.cidFlatten()

   Converts a CID font into a normal font (glyphs will be in CID order).

.. method:: font.cidFlattenByCMap(cmap_filename)

   Converts a CID font into a normal font using the encoding specified in the
   CMAP file.

.. method:: font.cidInsertBlankSubFont()

   Adds a new (blank) sub-font into a cid-keyed font and changes the current
   sub-font to be it.

.. method:: font.cidRemoveSubFont()

   Removes the current subfont from a cid-keyed font.

.. method:: font.close()

   Frees memory for the current font.

   **Warning:** Any python references to it will become invalid.

.. method:: font.compareFonts(other_font, filename, flags_tuple)

   This will compare the current font with the font in ``other-font`` (which
   must already have been opened). It will write the results to the
   ``filename``, you may use "-" to send the output to stdout. The ``flags``
   argument is a tuple of strings and controls what will be compared.

   .. object:: outlines

      compare outlines

   .. object:: outlines-exactly

      compare outlines exactly (otherwise allow slight errors and the unlinking
      of references)

   .. object:: warn-outlines-mismatch

      warn if the outlines don't exactly match (but are pretty close)

   .. object:: hints

      compare hints

   .. object:: warn-refs-unlink

      warn if references need to be unlinked before a match is found

   .. object:: strikes

      compare bitmap strikes

   .. object:: fontnames

      compare font names

   .. object:: gpos

      compare glyph positioning

   .. object:: gsub

      compare glyph substitutions

   .. object:: add-outlines

      for any glyphs whose outlines differ, add the outlines of the glyph in
      the second font to the background of the glyph in the first

   .. object:: create-glyphs

      if a glyph exists in the second font but not the first, create that
      glyph in the first and add the outlines from the second into the
      backgroun layer


.. method:: font.createChar(uni[, name])

   Create (and return) a character at the specified unicode codepoint in this
   font and optionally name it. If you wish to create a glyph with no unicode
   codepoint, set the first argument to -1 and specify a name.

   If there is already a character at that (positive) codepoint then it is
   returned. If the optional name parameter is included and differs from its
   current name then the character is also renamed.

.. method:: font.createInterpolatedGlyph(glyph1, glyph2, amount)

   Create (and return) a glyph with the same unicode code point as glyph1.
   The glyph may not already exist. The contents of the glyph will be formed
   by interpolating between glyph1 and glyph2. If amount==0 the result will
   look like glyph1, or 1 then like glyph2.

.. method:: font.createMappedChar(enc)
            font.createMappedChar(name)

   Create (and return) a character at the specified encoding in this font.
   If there is already a character there, return it

.. method:: font.find(contour[, error_bound, search_flags])

   Searches the font for all glyphs containing the contour (or layer) and
   returns an iterator which returns those glyphs.

   error-bound: defaults to 0.01.

   search_flags: tuple of the strings: reverse, flips, rotate, scale.

   When found, the glyph.temporary is set to a dict of:

   ::

      {
        "findMatchedRefs": matched_refs_bit_map,
        "findMatchedContours": matched_contours_bit_map,
        "findMatchedContoursStart": matched_contours_start_bit_map,
      }

.. method:: font.findEncodingSlot(uni)
            font.findEncodingSlot(name)

   Tests whether a glyph with this codepoint or name is in the font's encoding
   and returns the encoding slot. If the glyph is not present it returns -1.

   (If a glyph with that name/unicode is in the font, but is not in the
   encoding, then an value beyond the end of the encoding will be returned).

.. method:: font.glyphs([type])

   Returns an iterator which will return the glyphs in the font. By default
   they will be returned in "GID" order, but if type is specified as "encoding"
   they will be returned in encoding order. If returned in encoding order it
   is possible that a glyph will be returned more than once if there are
   multiple encoding slots which reference it.

.. method:: font.generate(filename[, bitmap_type=, flags=, bitmap_resolution=, subfont_directory=, namelist=, layer=])

   Generates a font. The type is determined by the font's extension. The bitmap
   type (if specified) is also an extension. If layer is specified, then the
   splines and references in that layer will be used instead of the foreground
   layer.

   Flags is a tuple containing some of

   .. object:: afm

      output an afm file

   .. object:: pfm

      output a pfm file

   .. object:: tfm

      output a tfm file

   .. object:: ofm

      output a ofm file

   .. object:: composites-in-afm

      Store composite info in the afm file

   .. object:: glyph-map-file

      Output a glyph map file giving the mapping between output gid and glyphnames

   .. object:: short-post

      Do not include glyphnames in a ttf/otf file

   .. object:: apple

      output apple advanced typography tables

   .. object:: opentype

      output opentype tables

   .. object:: old-kern

      output an old style 'kern' with opentype tables

   .. object:: winkern

      only add kern pairs for mapped glyphs (required for kerning in some/all
      versions of Windows)

   .. object:: dummy-dsig

      output an empty DSIG table so MS will mark a font with .ttf extension as
      an OpenType font.

   .. object:: no-FFTM-table

      Do not generate an 'FFTM' table

   .. object:: TeX-table

      Include a 'TeX ' table in an ttf/otf file

   .. object:: round

      Round PS coordinates to integers

   .. object:: no-hints

      Do not include PS hints

   .. object:: no-flex

      Do not include PS flex hints

   .. object:: omit-instructions

      Do not include TrueType instructions

   .. object:: PfEd-comments

      Include font and glyph comments in the 'PfEd' table

   .. object:: PfEd-colors

      Include glyph colors in the 'PfEd' table

   .. object:: PfEd-lookups

      Include lookup names in the 'PfEd' table

   .. object:: PfEd-guidelines

      Include guideline locations in the 'PfEd' table

   .. object:: PfEd-background

      Include background (and spiro) layers in the 'PfEd' table

   .. object:: symbol

      Generate an sfnt with a Symbol cmap entry rather than a Unicode entry.

   See also :meth:`font.save()`.

.. method:: font.generateTtc(filename, others, [flags=, ttcflags=,  namelist=, layer=])

   Generates a truetype collection file containing the current font and all
   others listed -- the ``others`` argument may be ``None``, a font, or a tuple
   (or list) of fonts.

   Flags are as above,

   Ttcflags is a tuple consisting of the following

   .. object:: merge

      Try and share tables and glyphs among the various fonts.

   .. object:: cff

      Use the CFF glyph format rather than the TrueType format (the OpenType
      documentation says that this does not work, but both the Mac and
      unix/linux accept it).

.. method:: font.generateFeatureFile(filename[, lookup_name])

   Generates an adobe feature file for the current font. If a lookup-name is
   specified then only data for that lookup will be generated.

.. method:: font.genericGlyphChange(stemType=<str>, thickThreshold=<double>, stemScale=<double>, stemAdd=<double>, stemHeightScale=<double>, stemHeightAdd=<double>, stemWidthScale=<double>, stemWidthAdd=<double>, thinStemScale=<double>, thinStemAdd=<double>, thickStemScale=<double>, thickStemAdd=<double>, processDiagonalStems=<boolean>, hCounterType=<str>, hCounterScale=<double>, hCounterAdd=<double>, lsbScale=<double>, lsbAdd=<double>, rsbScale=<double>, rsbAdd=<double>, vCounterType=<str>, vCounterScale=<double>, vCounterAdd=<double>, vScale=<double>, vMap=<tuple of tuples>)

   This function uses keyword parameters. Which ones are required depends on
   the three type arguments (``stemType, hCounterType, vCounterType``).

   If ``stemType`` is omitted, or is the string "uniform", then the
   ``stemScale`` parameter must be specified (and ``stemAdd`` may be).
   ``stemScale`` specifies a scaling factor by which all stems (horizontal and
   vertical, thick and thin) will be scaled. A value of 1.0 means no change.
   While ``stemAdd`` specifies the number of em-units to add to the width of
   each stem.

   If ``stemType`` is the string "horizontalVertical", then values must be
   specified for ``stemHeightScale`` and ``stemWidthScale`` (and may be for
   ``stemHeightAdd, stemWidthAdd`` ). The first of these specifies scaling for
   the height of horizontal stems, and the second scaling for the width of
   vertical stems.

   If ``stemType`` is the string "thickThin", then values must be specified for
   ``thinStemScale``, ``thickStemScale`` and ``thickThreshold`` (and may be for
   ``thinStemAdd, thickStemAdd`` ). The first of these specifies scaling for
   the width/height of thin stems, and the second scaling for the width/height
   of thick stems. While the ``thickThreshold`` argument specifies the size (in
   em-units) at which a stem is classified as "thick".

   If ``hCounterType`` is omitted, or is the string "uniform", then horizontal
   counters, and the left and right side bearings will all be scaled using the
   same rules, and ``hCounterScale`` must be specified to provide the scaling
   factor (while ``hCounterAdd`` may be specified).

   If ``hCounterType`` is the string "nonUniform", then horizontal counters,
   and the left and right side bearings may all be scaled using different
   rules, and ``hCounterScale, lsbScale`` and ``rsbScale`` must be specified
   to provide the scaling factors (while ``hCounterAdd, lsbAdd,`` and
   ``rsbAdd`` may be specified).

   If ``hCounterType`` is the string "center", then the left and right
   side-bearings will be set so the new glyph is centered within the original
   glyph's width. (Probably more useful for CJK fonts than LGC fonts).

   If ``hCounterType`` is the string "retainScale", then the left and right
   side-bearings will be set so the new glyph is within within the original
   glyph's width, and the side-bearings remain in the same proportion to each
   other as before.

   If ``vCounterType`` is omitted, or is the string "mapped", then certain
   zones on the glyph may be placed at new (or the same) locations -- similar
   to BlueValues. So you can specify a zone for the baseline, one for the
   x-height and another for the top of capitals and ascenders (and perhaps a
   fourth for descenders). Each such zone is specified by the ``vMap`` argument
   which is a tuple of 3-tuples, each 3-tuple specifying a zone with: Original
   location, original width, and final location.

   .. note::

      No default value is providedfor this argument you must figure out all the
      values yourself.

   If ``vCounterType`` is the string "scaled", then vertical counters, and the
   top and bottom side bearings will all be scaled using the same rules, and
   ``vCounterScale`` must be specified to provide the scaling factor (while
   ``vCounterAdd`` may be specified). This is probably most useful for CJK fonts.

.. method:: font.getKerningClass(subtable_name)

   Returns a tuple whose entries are: (first-classes, second-classes, offsets).
   The classes are themselves tuples of tuples of glyph names. The offsets will
   be a tuple of numeric kerning offsetss a tuple whose entries are:
   (first-classes, second-classes, offsets). The classes are themselves tuples
   of tuples of glyph names. The offsets will be a tuple of numeric kerning
   offsets.

.. method:: font.getLookupInfo(lookup_name)

   Returns a tuple whose entries are: (lookup-type, lookup-flags,
   feature-script-lang-tuple). The lookup type is a string as described in
   :meth:`font.addLookup`, and the feature-script-lang tuple is also described
   there.

.. method:: font.getLookupSubtables(lookup_name)

   Returns a tuple of all subtable names in that lookup.

.. method:: font.getLookupSubtableAnchorClasses(subtable_name)

   Returns a tuple of all anchor class names in that subtable.

.. method:: font.getLookupOfSubtable(subtable_name)

   Returns the name of the lookup containing this subtable.

.. method:: font.getSubtableOfAnchor(anchor_class_name)

   Returns the name of the subtable containing this anchor class.

.. method:: font.importBitmaps(bitmap_font_file[, to_background])

   Load any bitmap strikes out of the bitmap-font-file into the current font

.. method:: font.importLookups(another_font, lookup_names[, before_name])

   The first argument must be a :class:`font` object, the second a string or a
   tuple of strings, and the third, another string.

   It will search the other font for the named lookup(s) and import it into the
   current font. (Contextual lookups which invoke other lookups will have any
   nested lookups imported as well).

   Lookups will be imported in the order listed. If a before-name is specified,
   then it is looked up in the current font and all lookups will be added
   before it, if not specified lookups will appear at the end of the list.

.. method:: font.interpolateFonts(fraction, filename)

   Interpolates a font between the current font and the font contained in
   filename.

.. method:: font.isKerningClass(subtable_name)

   Returns whether the named subtable contains a kerning class.

.. method:: font.isVerticalKerning(subtable_name)

   Returns whether the named subtable contains a vertical kerning data

.. method:: font.italicize(italic_angle=, ia=lc_condense=, lc=uc_condense=, uc=symbol_condense=, symbol=deserif_flat=, deserif_slant=, deserif_pen=, baseline_serifs=, xheight_serifs=, ascent_serifs=, descent_serifs=, diagonal_serifs=, a=, f=, u0438=, u043f=, u0442=, u0444=, u0448=, u0452=, u045f=)

   This function uses keyword parameters. None are required, if omitted a
   default value will be used. Some keywords have abbreviations ("ia" for
   "italic_angle") you may use either.

   This function will attempt to italicize each selected glyph. For a detailed
   explanation of what this entails please see the information on the
   :ref:`Italic dialog <Styles.Italic>`.

   The ``*_condense`` keywords should be 4 element tuples of floating point
   numbers; these numbers correspond to: Left side bearing condensation,
   stem condensation, counter condensation and right side bearing condensation.
   These numbers should be small numbers around 1 (scale factors, not
   percentages).

   Set at most one of the ``deserif_*`` keywords.

   Setting ``a`` to ``True`` will turn on the transformation that applies to
   the "a" glyph. Setting ``u0438`` will control the transformation that
   applies to the glyph at unicode codepoint U+0438.

   The ``f`` keyword is slightly more complex. Setting it to 0 turns off all
   transformations of glyphs like "f", setting it to 1 will give "f" a tail
   which looks like a rotated version of its head, and setting it to 2 will
   simply extend the main stem of "f" below the baseline.

.. method:: font.lookupSetFeatureList(lookup_name, feature_script_lang_tuple)

   Sets the feature list of indicated lookup. The feature-script-lang tuple is
   described at :meth:`font.addLookup()`.

.. method:: font.lookupSetFlags(lookup_name, flags)

   Sets the lookup flags for the named lookup.

.. method:: font.lookupSetStoreLigatureInAfm(lookup_name, boolean)

   Sets whether this ligature lookup contains data to store in the afm.

.. method:: font.mergeFonts(filename[, preserveCrossFontKerning])(font[, preserveCrossFontKerning])

   Merges the font in the file into the current font.

.. method:: font.mergeFeature(filename)

   Merge feature and lookup information from an adobe feature file, or metrics
   information from the (afm,tfm,etc) file into the current font.

.. method:: font.mergeKern(filename)

   Deprecated name for mergeFeature above

.. method:: font.mergeLookups(lookup_name1, lookup_name2)

   The lookups must be of the same type. All subtables from lookup_name2 will
   be moved to lookup_name1, the features list of lookup_name2 will be merged
   with that of lookup_name1, and lookup_name2 will be removed.

.. method:: font.mergeLookupSubtables(subtable_name1, subtable_name2)

   The subtables must be in the same lookup. Not all lookup types allow their
   subtables to be merged (contextual subtables may not be merged, kerning
   classes may not be (kerning pairs may be)). Any information bound to
   subtable2 will be bound to subtable1 and subtable2 will be removed.

.. method:: font.printSample(type, pointsize, sample, output_file)

   Type is a string which must be one of

   .. object:: fontdisplay

      Display all glyphs in the font in encoding order

   .. object:: chars

      Display the selected glyphs scaled to fill a page

      Ignores the pointsize argument.

   .. object:: waterfall

      Displays the selected glyphs at many pointsizes.

      The pointsize argument should be a tuple of pointsizes here.

   .. object:: fontsample

      The third argument should contain a string which will be layed out
      and displayed as well as FontForge can.

   .. object:: fontsampleinfile

      The third argument should contain the name of a file which contains
      text to be layed out and displayed.

   If output is to a file (see :func:`fontforge.printSetup()`) then the last
   argument specifies a file name in which to store output.

.. method:: font.randomText(script[, lang])

   Returns a random text sample using the letter frequencies of the specified
   script (and optionally language). Both script and language should be
   expressed as strings containing OpenType Script and Language tags. "dflt" is
   a reasonable language tag. If the language is not specified, one will be
   chosen at random. If ff has no frequency information for the script/language
   specified it will use the letters in the script with equal frequencies.

.. method:: font.regenBitmaps(tuple_of_sizes)

   A tuple with an entry for each bitmap strike to be regenerated
   (rerasterized). Each strike is identified by pixelsize (if the strike is a
   grey scale font it will be indicated by ``(bitmap-depth<<16)|pixelsize``.

.. method:: font.removeAnchorClass(anchor_class_name)

   Removes the named AnchorClass (and all associated points) from the font.

.. method:: font.removeLookup(lookup_name[, remove_acs])

   Remove the lookup (and any subtables within it). remove_acs (0 or 1),
   specifies whether to remove associated anchor classes and points.

.. method:: font.removeLookupSubtable(subtable_name[, remove_acs])

   Remove the subtable (and all data associated with it). remove_acs (0 or 1),
   specifies whether to remove associated anchor classes and points

.. method:: font.removeGlyph(uni[, name])(name)(glyph)

   You may either pass in a FontForge glyph object (from this font) or identify
   a glyph in the font by unicode code point or name. In any case the glyph
   will be removed from the font.

   If you use (uni,name) to specify a name, set uni to -1.

   .. warning::

      This frees FontForge's storage to this glyph. If you have any python
      references to that storage they will be looking at garbage. This does not
      go through the usual python reference mechanism.

.. method:: font.replaceAll(srch, rpl[, error_bound])

   Searches the font for all occurences of the srch contour (or layer) and
   replaces them with the replace contour (or layer).

.. method:: font.revert()

   Reloads the font from the disk.

   .. warning::

      If you have any references to glyphs which live in the font those
      references will no longer be valid, and using them will cause crashes.
      This is very un-python-like.

.. method:: font.revertFromBackup()

   Reloads the font from the backup file on the disk.

   .. warning::

      If you have any references to glyphs which live in the font those
      references will no longer be valid, and using them will cause crashes.
      This is very un-python-like.

.. method:: font.save([filename])

   Saves the font to an sfd file. See also :meth:`font.generate()`

.. method:: font.saveNamelist(filename)

   Saves the font's namelist to a file.

.. method:: font.getTableData(table_name)

   Gets binary data from any saved table. FF will save 'fpgm', 'prep', 'cvt '
   and 'maxp'. FF may also save tables which you explicitly request. Do not
   expect to get binary data for tables like 'GPOS' or 'glyf' which FF will
   generate when it creates a font... that information is not currently available.

   Returns a binary string.

.. method:: font.setTableData(table_name, sequence)

   Sets binary data of any saved table. FF will save 'fpgm', 'prep', 'cvt '
   and 'maxp'. FF may also save tables which you explicitly request. Do not
   expect to set binary data for tables like 'GPOS' or 'glyf' which FF will
   generate when it creates a font... that information is not currently available.

   If sequence is None, then the named table will be removed from the font.

.. method:: font.validate([force])

   Validates the font and returns a bit mask of all errors from all glyphs (as
   defined in the ``validation_state`` of a glyph -- except bit 0x1 is clear).
   If the font passed the validation then the return value will be 0 (not 0x1).
   Otherwise the return value will be the set of errors found.

   Note: The set of errors is slightly different for TrueType and PostScript
   output. The returned mask contains the list of potential errors. You must
   figure out which apply to you.

   Normally each glyph will cache its validation_state and it will not be
   recalculated. If you pass a non-zero argument to the routine then it will
   force recalculation of each glyph -- this can be slow.


.. rubric:: Selection Based Interface

See the :class:`selection` type for how to alter the selection.


.. method:: font.addExtrema()

   Extrema should be marked by on-curve points. If a curve in any selected
   glyph lacks a point at a significant extremum this command will add one.

.. method:: font.autoHint()

   Generates PostScript hints for all selected glyphs.

.. method:: font.autoInstr()

   Generates TrueType instructions for all selected glyphs.

.. method:: font.autoWidth(separation[, minBearing=, maxBearing=, height=, loopCnt=])

   Guesses at reasonable horizontal advance widths for the selected glyphs

.. method:: font.autoTrace()

   Auto traces any background images in all selected glyphs

.. method:: font.build()

   If any of the selected characters is a composite character, then this
   command will clear it and insert references to its components (this command
   can create new glyphs).

.. method:: font.canonicalContours()

   Orders the contours in the selected glyphs by the x coordinate of their
   leftmost point. (This can reduce the size of the charstring needed to
   describe the glyph(s).

.. method:: font.canonicalStart()

   Sets the start point of all the contours of the selected glyphs to be the
   leftmost point on the contour. (If there are several points with that value
   then use the one which is closest to the baseline). This can reduce the size
   of the charstring needed to describe the glyph(s). By regularizing things it
   can also make more things available to be put in subroutines.

.. method:: font.changeWeight(stroke_width[, type, serif_height, serif_fuzz, counter_type, custom_zones])

   See the :ref:`Element->Style->Change Width <Styles.Embolden>` command for a
   more complete description of these arguments.

   Stroke_width is the amount by which all stems are expanded.

   Type is one of "LCG", "CJK", "auto", "custom".

   Serif_height tells ff not to expand serifs which are that much off the
   baseline, while serif_fuzz specifies the amount of fuzziness allowed in the
   match. If you don't want special serif behavior set this to 0.

   Counter_type is one of "squish", "retain", "auto".

   Custom_zones is only meaningful if the type argument were "custom". It may
   be either a number, which specifies the "top hint" value (bottom hint is
   assumed to be 0, others are between), or a tuple of 4 numbers (top hint,
   top zone, bottom zone, bottom hint).

.. method:: font.condenseExtend(c_factor, c_add[, sb_factor, sb_add, correct])

   Condenses or extends the size of the counters and side-bearings of the
   selected glyphs. The first two arguments provide information on
   shrinking/growing the counters, the second two the sidebearings. If the last
   two are omitted they default to the same values as the first two.

   A counter's width will become: ::

      new_width = c_factor * old_width + c_add

   If present the ``correct`` argument allows you to specify whether you want
   to correct for the italic angle before condensing the glyph. (it defaults to
   True)

.. method:: font.clear()

   Clears the contents of all selected glyphs

.. method:: font.cluster([within, max])

   Moves clustered coordinates to a standard central value in all selected
   glyphs. See also :meth:`font.round()`.

.. method:: font.copy()

   Copies all selected glyphs into (FontForge's internal) clipboard.

.. method:: font.copyReference()

   Copies all selected glyphs (as references) into (FontForge's internal)
   clipboard.

.. method:: font.correctDirection()

   Orients all contours so that external ones are clockwise and internal
   counter-clockwise in all selected glyphs.

.. method:: font.correctReferences()

   Checks a font for glyphs with mixed contours and references (or references
   with transformation matrices which cannot be represented truetype (ie.
   scaling by 2 or more)). If a mixed case is discovered FontForge will take
   the contours out of the glyph, put them in a new glyph, and make a reference
   to the new glyph.

.. method:: font.cut()

   Copies all selected glyphs into (FontForge's internal) clipboard. And then
   clears them.

.. method:: font.paste()

   Pastes the contents of (FontForge's internal) clipboard into the selected
   glyphs -- and removes what was there before.

.. method:: font.intersect()

   Leaves only areas in the intersection of contours in all selected glyphs.
   See also :meth:`font.removeOverlap()`.

.. method:: font.pasteInto()

   Pastes the contents of (FontForge's internal) clipboard into the selected
   glyphs -- and retains what was there before.

.. method:: font.removeOverlap()

   Removes overlapping areas in all selected glyphs.
   See also :meth:`font.intersect()`.

.. method:: font.replaceWithReference([fudge])

   Finds any glyph which contains an inline copy of one of the selected glyphs,
   and converts that copy into a reference to the appropriate glyph. Selection
   is changed to the set of glyphs which the command alters.

   If specified the fudge argument specifies the error allowed for coordinate
   differences.

.. method:: font.round([factor])

   Rounds the x and y coordinates of each point in all selected glyphs. If
   factor is specified then ::

      new-coord = round(factor*old-coord)/factor

   See also :meth:`font.cluster()`.

.. method:: font.simplify([error_bound, flags, tan_bounds, linefixup, linelenmax])

   Tries to remove excess points in all selected glyphs if doing so will not
   perturb the curve by more than ``error-bound``. Flags is a tuple of the
   following strings

   .. object:: ignoreslopes

      Allow slopes to change

   .. object:: ignoreextrema

      Allow removal of extrema

   .. object:: smoothcurves

      Allow curve smoothing

   .. object:: choosehv

      Snap to horizontal or vertical

   .. object:: forcelines

      flatten bumps on lines

   .. object:: nearlyhvlines

      Make nearly horizontal/vertical lines be so

   .. object:: mergelines

      Merge adjacent lines into one

   .. object:: setstarttoextremum

      Rotate the point list so that the start point is on an extremum

   .. object:: removesingletonpoints

      If the contour contains just one point then remove it

.. method:: font.stroke("circular", width[, CAP, JOIN, FLAGS])
            font.stroke("elliptical", width, minor_width, ANGLE[, CAP, JOIN, FLAGS])
            font.stroke("calligraphic", width, height, angle[, FLAGS])
            font.stroke("polygon", contour[, FLAGS])
   (Legacy interface)
   :noindex:

.. method:: font.stroke("circular", width[, CAP, JOIN, ANGLE, KEYWORD])
            font.stroke("elliptical", width, minor_width[, ANGLE, CAP, JOIN, KEYWORD])
            font.stroke("calligraphic", width, height[, ANGLE, CAP, JOIN, KEYWORD])
            font.stroke("convex", contour[, ANGLE, CAP, JOIN, KEYWORD])
   (Current interface)

   Strokes the lines of the contours in all selected glyphs according to the
   supplied parameters. See :meth:`glyph.stroke()` for a description of the
   syntax and the :doc:`stroke </techref/stroke>` documentation for more general
   information.

.. method:: font.transform(matrix)

   Transforms all selected glyphs by the matrix.

.. method:: font.nltransform(xexpr, yexpr)

   xexpr and yexpr are strings specifying non-linear transformations that will
   be applied to all points in the selected glyphs of the font (with xexpr
   being applied to x values, and yexpr to y values, of course). The syntax
   for the expressions is explained in the
   :ref:`non-linear transform dialog <transform.Non-Linear>`.

.. method:: font.unlinkReferences()

   Unlinks all references in all selected glyphs and replaces them with splines.

