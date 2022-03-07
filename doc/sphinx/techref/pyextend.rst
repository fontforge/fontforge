Extending FontForge With Python
===============================

FontForge includes an extensive :doc:`Python API </scripting/python>` which is
most often used in scripts to alter and generate font files. However, this API
also provides ways of extending the functionality of the FontForge program,
including the user interface. Most of these are documented in the :ref:`User
Interface Module Functions <fontforge.ui_functions>` section of the Python
documentation.

Modifying Points and Contours, References, and Anchor Points
------------------------------------------------------------

Given that points and contours are basis of vector font data many extensions
will want to modify them. There is one misleading aspect of the "layer" API
that must be clarified in order to do so. It is also important to be aware
of layers and how they relate to the FontForge user interface.

The Basics
^^^^^^^^^^

A ``point`` is (typically) an element of a ``contour`` and a ``contour`` is
(typically) an element of a ``layer``. Confusingly, however, there are really
two meanings of the term "layer" in the FontForge Python API. One is the
``fontforge.layer`` object, which is the object that can contain
``fontforge.contour`` objects. The other is a layer *internal* to a font
object.

The confusion arises when a developer attempts to modify a point in this way: ::

    f = fontforge.open('foo.otf')
    g = f['a']
    l = g.foreground
    for c in l:
        for p in c:
            p.x += 15
    f.generate('foo_new.otf')

After this script executes the points of the glyph "a" in ``foo_new.otf`` will
be the same as those in ``foo.otf``. And this is true even though the points in
the ``fontforge.layer`` object ``l`` have been changed. This is because ``l``
is not a *reference* to ``g.foreground`` (which represents the internal state
of the layer) but a *copy* of it. Any changes to the layer must therefore be
"copied back" to have an effect on the font. This can be done with an
assignment but we now recommend using the ``glyph.setLayer()`` method instead,
as this gives you more control over how point types are preserved or converted
as they are copied back.::

    f = fontforge.open('foo.otf')
    g = f['a']
    l = g.foreground
    for c in l:
        for p in c:
            p.x += 15
    g.setLayer(l, 'Fore')
    f.generate('foo_new.otf')

Layer Awareness and Undo Preservation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The author of a "top-down" script for modifying a font will typically know what
layer to modify—most often ``glyph.foreground`` (which is also identified as
"0" or "Fore"). A script that extends the FontForge UI with a new menu option
should not make such assumptions. At a given time the user may be editing the
foreground or background layer or even one of a few layers in a multi-layer
font. Such a function should therefore typically not reference specific layers
at all and use ``glyph.activeLayer`` instead, as in: ::

    def shiftBy15(u, glyph):
        l = glyph.layers[glyph.activeLayer]
        for c in l:
            for p in c:
                p.x += 15
        glyph.preserveLayerAsUndo(glyph.activeLayer)
        g.setLayer(l, glyph.activeLayer)

This function also demonstrates an appropriate use of the
``preserveLayerAsUndo()`` method. Setting an "internal" layer (either through
assignment for with the ``glyph.setLayer()`` method) does not automatically add
the previous layer to the Undo/Redo state. By calling
``glyph.preserveLayerAsUndo()`` you give the user the option of reversing your
change with "Edit -> Undo".

References
^^^^^^^^^^

The reference API is similar to the layer API, with the same caveats. You can
get the list of references for the current UI layer with ``rl =
g.layerrefs[g.activeLayer]`` (where ``g`` is a ``fontforge.glyph`` object).  To
effect changes in the references you need to assign back to
``g.layerrefs[g.activeLayer]``. Each reference is represented by a tuple of
glyph name, transformation matrix tuple (that is, a sub-tuple of six floating
point numbers representing the transformation) and a boolean ``selected`` status.
This script accordingly shifts the references in the current layer by 15 points: ::

    def shiftRefsBy15(u, glyph):
        rl = glyph.layerrefs[glyph.activeLayer]
        nl = []
        tr = psMat.translate(15,0)
        for r in rl:
            nl.append((r[0], psMat.compose(r[1], tr), r[2]))
        glyph.preserveLayerAsUndo(glyph.activeLayer)
        glyph.layerrefs[glyph.activeLayer] = nl

(One could also have used a transformation matrix to move the points in the earlier
fragments by passing it to ``p.transform()``.)

As noted in the API documentation, when assigning to the reference list the
transformation matrix and selection status tuple elements are optional (with
respective defaults of the identity matrix and ``False``).

Anchor Points
^^^^^^^^^^^^^

In contrast with points and references, all anchor points are considered part
of the foreground layer. They can be accessed and edited through the
``glyph.anchorPointsWithSel`` property. When editing the list care must be
taken to construct the tuples so that the name, type, position, selection and
(when relevant) ligature information are preserved or changed as desired.

.. _fontforge.plugin_menu:

Adding a Menu Item
------------------

The functions ``shiftBy15()`` and ``shiftRefsBy15()`` above were written to
be added as menu items using :py:meth:`fontforge.registerMenuItem`.  This is
the most common form of FontForge UI extension and we recommend familiarizing
(or re-familiarizing) yourself with the documentation as the API has recently
been enhanced.

Selection State
^^^^^^^^^^^^^^^

In the case of many menu items, probably most of them, the user should have
some "input" into how one applies in a given situation. Sometimes this is a
reason to raise a dialog asking the user for input, but in the majority of
cases a menu callback will "interact" with a user by way of *selection state*.

Each point, both on-curve and control, has a boolean selection state. The
selection state of on-curve points is generally more important or "primary".
The "Transform ..." dialog, for example, transforms selected on-curve points
(and references) and their associated off-curve points regardless of the
latter's selection status. (This is true unless no points are selected, in
which case all points are transformed.) Other contour-level facilities, such as
"Correct Direction", consider a contour to be selected if at least one of its
on-curve points is selected, and changes those contours accordingly.

Still, in some cases you may want to attend to the selection state of
control points.

Each reference and anchor point also has a selection state. The Python
reference API has recently been enhanced to make the state available.

As an example, consider the ``shiftRefsBy15`` function above. We could
enhance it to shift only *selected* references unless there are none,
in all references are shifted: ::

    def shiftRefsBy15(u, glyph):
        rl = glyph.layerrefs[glyph.activeLayer]
        has_selected = False
        for r in rl:
            if r[2]:
                has_selected = True
        nl = []
        tr = psMat.translate(15,0)
        for r in rl:
            if not has_selected or r[2]:
                nl.append((r[0], psMat.compose(r[1], tr), r[2]))
            else:
                nl.append(r)
        glyph.preserveLayerAsUndo(glyph.activeLayer)
        glyph.layerrefs[glyph.activeLayer] = nl

At the font level each font has its own selection state accessible via the
:py:attr:`fontforge.font.selection` property. Many useful font-level tools will
be selection-agnostic but some should only apply to selected glyphs.  When
these are added as menu items they should include enable_functions analogous to
those of a glyph-level tool, so that a user learns whey they do and do not
apply.

Enable Functions
^^^^^^^^^^^^^^^^

Some menu items, such as one that adds a new point in a CharView, may always be
"applicable". Others can only do something sensible if the character (or font)
has certain features, such as at least one point or reference, or if certain
combinations of points, anchor points, and references are selected. When adding
a new option you should help the user understand when it is and is not
applicable by writing and registering an ``enable`` function. This will allow
FontForge to display the item as enabled in the menu when it applies and as
disabled when it does not apply.

The enhanced version of ``shiftRefsBy15()`` only applies if there is at least
one reference, so it's enable function could be: ::

    def SRB15Enable(u, glyph):
        return len(glyph.layerrefs[glyph.activeLayer])>0

See the next section for an example of how to register this function.

Hotkeys and Mnemonics
^^^^^^^^^^^^^^^^^^^^^

A menu item can have both a Hotkey and a Mnemonic. These are two different means
of picking a menu item using the keyboard.

A Hotkey is a single key combination, usually including modifiers like Control,
Alt, and Shift, that directly invokes a menu item or other action. Because
hotkeys are a "limited resource", unless you are confident that a menu item you
register should have one it may be better to let the user add their own if they
find your addition particularly useful. As the
:py:meth:`fontforge.registerMenuItem` documentation notes, even if you do
include a ``hotkey`` string when registering that key combination may already
be taken, in which case no hotkey will be assigned.

Mnemonics provide a *per-menu* key accelerator, usually activated by pressing
Alt and then the mnemonic key displayed with an underline. Using sequences of
mnemonic key combinations a user can navigate from the menu bar down through
sub-menus and choose a menu action entirely with the keyboard. Mnemonics are also
a "limited resource" but *less* limited, and we encourage all plugin developers to
specify them.

You specify a mnemonic key by preceding it with an underscore in the name, as in: ::

    fontforge.registerMenuItem(callback=shiftRefsBy15, enable=SRB15Enable,
            context=("Glyph"), name=("_Shift References by 15", "MyExt.shiftRefsBy15"),
            submenu=("_MyExt", "MyExt.submenu"))

which specifes "M" as the mnemonic for the "MyExt" submenu and "S" as the mnemonic for
the action.

Note, however, that the specified mnemonic for the top level is only taken as a
suggestion, and if it is not available another will be assigned. This is
because different users will have different combinations of plugins and init
scripts installed in different orders, so any given mnemonic may already be
in use. Because lower menu levels will typically have entries for one plugin
or script you can count on getting the mnemonic you specify.

Best Practices
^^^^^^^^^^^^^^

In addition to the advice above, we recommend that a plugin put all of its
added menu items into or under a single sub-menu of "Tools". A user may have
many plugins installed or have their own init scripts so a single plugin should
avoid taking up a lot of space.

Storing Per-Font and Per-Glyph Information
------------------------------------------

The FontForge SFD file format and python API includes
:py:attr:`fontforge.font.persistent` for storing per-font information and
:py:attr:`fontforge.glyph.persistent` for storing per-glyph information.
These are fragile interfaces, however, and should only be used when strictly
necessary.

The FontForge program does not enforce any pattern of use on these attributes.
We suggest the following conventions:

1. If the contents are *empty*, create a dictionary and add an entry to it
   with your plugin name as the key. Store your data in or "under" the value
   of that entry. Then assign it to the ``persistent`` attribute.
2. If the content is a dictionary add your entry to it if it is not already
   present and store the result.
3. If the content is not a dictionary warn the user with an
   :py:meth:`fontforge.ask` dialog that allows them to opt-out of overwriting
   the value. If they give permission you can create a dictionary as in step
   1 and overwrite the existing data.

.. _fontforge.plugin_api:

Writing a FontForge Plugin
--------------------------

Even when you eventually plan on writing a FontForge Python plugin it will
generally be easiest to start by writing an "Init Script". This is a script
that, when placed in one of the directories listed by the
:py:meth:`fontforge.scriptPath() <fontforge.scriptPath>` function, is
automatically run when FontForge is started.

As a simple example, here is a script that adds an "Add Midpoint Contour" entry
to the Char View Tools menu. When any two on-curve points of a glyph are
selected this adds a new contour containing a single point that is located
midway between the two selected points.::

    import fontforge

    def getSelectedPoints(l):
        pl = []
        for c in l:
            pl.extend( [ p for p in c if p.on_curve and p.selected ] )
        return pl

    def midContourEnable(u, glyph):
        return len(getSelectedPoints(glyph.layers[glyph.activeLayer]))==2

    def addMidContour(u, glyph):
        layer_id = glyph.activeLayer
        l = glyph.layers[layer_id]
        pl = getSelectedPoints(l)
        if len(pl) != 2:
            fontforge.postError("Bad selection", "You must select "
            "exactly two on-curve points to add a midpoint contour.")
            return
        nc = fontforge.contour()
        nc.insertPoint(((pl[0].x+pl[1].x)/2, (pl[0].y+pl[1].y)/2, True,
                fontforge.splineCorner, True))
        l += nc
        glyph.preserveLayerAsUndo(layer_id)
        glyph.layers[layer_id] = l

    fontforge.registerMenuItem(callback=addMidContour, enable=midContourEnable,
            context=("Glyph"), name="_Add Midpoint Contour")

This script is typical in that it starts with some imports, defines some
functions, and ends by invoking :py:meth:`fontforge.registerMenuItem`. Other
scripts may import other modules, define more elaborate functions or classes,
or register more menu items or other callbacks. Still, this is a common
general pattern.

The first step in converting the script to a plugin is to wrap the last
section in a function called ``fontforge_plugin_init()``. Once the file
is packaged appropriately this function will be called after FontForge
discovers and loads the plugin (if the user has Enabled it).::

    def fontforge_plugin_init(**kw):
        fontforge.registerMenuItem(callback=addMidContour,
                enable=midContourEnable, context=("Glyph"),
                name="_Add Midpoint Contour")

The ``**kw`` function argument will capture any keyword arguments passed
to the function. There is currently one, discussed below, but more may
be added in the future. (Note that while the function can be located
in other places (see the section on Entry-points below) it must be a
*function*, not a *method*.

The remaining steps have to do with python "packaging", which is too
large a topic to discuss extensively in this guide. A good starting
point is `Packaging Python Projects
<https://packaging.python.org/tutorials/packaging-projects/>`_ from the
Python documentation. The following are the very basics.

First we create a new directory for our project with the following files:

1. ``fontforge_midpoint.py`` (The script itself as a single module file.)
2. ``LICENSE`` (Containing the license text.)
3. ``README.md`` (A file containing basic documentation for the plugin, in this
   case written in `Python-Mark-down format
   <https://pypi.org/project/Markdown/>`_.
4. ``pyproject.toml``
5. ``setup.cfg`` or ``setup.py``

``pyproject.toml`` is a short helper file for the ``setuptools`` package
builder. It will typically have these contents: ::

    [build-system]
    requires = [
        "setuptools>=42",
        "wheel"
    ]
    build-backend = "setuptools.build_meta"

Finally there is the ``setup.cfg`` file (or, less typically these days, the
``setup.py`` file). It should have contents analogous to these: ::

    [metadata]
    name = fontforge-midcontour
    version = 1.0.0
    author = Example Author
    author_email = author@example.com
    description = A FontForge_plugin to add a single-point contour between points
    long_description = file: README.md
    long_description_content_type = text/markdown
    url = https::/github.com/author/MidContour
    classifiers =
        Programming Language :: Python :: 3
        License :: OSI Approved :: MIT License
        Operating System :: OS Independent
        Topic :: Text Processing :: Fonts

    [options]
    py_modules = fontforge_midcontour
    python_requires = >=3.6

    [options.entry_points]
    fontforge_plugin =
        MidContour = fontforge_midcontour

The ``name`` field is the name of the *package*. Recent Python documentation
encourages developers to add their PyPI username to the package name but
this practice has not been widely adopted. For now we recommend that you
include "fontforge" in the package name along with a string that either
specifically identifies what your plugin does or is just a distinctive
project name. Therefore ambiguous names like "Point" or "Contour" should
be avoided as these are more likely to also be used by someone else. Please
search the PyPI database for a candidate name to help ensure it is not already
in use. Package names should be all lower case but can contain dashes and
underscores.

The ``url`` should point to your project page or a GitHub (or other such
service) repository for the plugin code.

Obviously the ``description`` field should contain a one-line description of
the plugin. However, it is important that this description (or, if that is not
possible, the README file) contain the exact string "FontForge_plugin".
FontForge does not maintain its own database of plugins; it instead links to
the PyPI database query system passing that string.

``classifiers`` help users people searching on PyPI to find
relevant packages (although the list of classifiers is, unfortunately, fixed).
This is a reasonable selection.

``py_modules`` should be the list of modules provided by the package without
any ``.py`` extensions. (More complicated packages with sub-directories
could benefit from using ``packages = find:`` instead.

The last directives specify the Python package "Entry-points", which are
the basis of FontForge's discovery system. The Entry-point identifier
must be ``fontforge_plugin``.

The token on the left of the indented line is the *plugin name*. This will
appear in the "Configure Plugins..." dialog and other contexts. It will
normally be the name of your package except without "fontforge" and with
optional capitalization. It can contain the usual
alphanumerics-plus-underscores and also spaces (although spaces are not
recommended).

The token on the right of the equals sign identifies the location of the
``fontforge_plugin_init`` function. In this case it is just at the top
level of the module so the identifier is just the module name. If it were
instead a property of object "MC" in that module the string would be
``fontforge_midcontour.MC``.

In some cases you may want to use a ``setup.py`` file instead. The file
equivalent to the ``setup.cfg`` above is: ::

   import setuptools

   with open("README.md", "r", encoding="utf-8") as fh:
       long_description = fh.read()

   setuptools.setup(
       name="fontforge_midcontour-author",
       version="1.0.0",
       author="Example Author",
       author_email="author@example.com",
       description="A FontForge_plugin to add a single-point contour between points",
       long_description = long_description,
       long_description_content_type = "text/markdown",
       url="https::/github.com/author/MidContour",
       py_modules=["fontforge_midcontour"],
       classifiers=[
           "Programming Language :: Python :: 3",
           "License :: OSI Approved :: MIT License",
           "Operating System :: OS Independent",
           "Topic :: Text Processing :: Fonts",
       ],
       python_requires=">=3.6",
       entry_points={ 'fontforge_plugin': [ 'MidContour = fontforge_midcontour' ],},
   )

Once the directory has the five files with the appropriate contents you can
build the package by entering the directory and running ``python -m build`` .
If there are no errors the package archive will be added to the ``dist``
subdirectory (which will be created if necessary). This package can be
installed directly with ``pip install [name]`` or `published on PyPI
<https://realpython.com/pypi-publish-python-package/#publishing-to-pypi>`_.

Marking Dependencies
^^^^^^^^^^^^^^^^^^^^

A simple plugin may have no dependencies beyond FontForge itself, and because
only FontForge will attempt to discover and load plugins it is not necessary
to mark that dependency. Other plugins may import numerical processing modules
or use ``tkinter`` to raise dialogs.

"Hard" dependencies—those that a plugin cannot operate without—should be marked
in the ``[options]`` section of ``setup.cfg`` with an ``install_requires``
directive. If any version of the package will suffice just use its name. If
your code requires a minimum version add ``>=`` and then that version. For
example, these directives specify that ``tkinter`` and ``numpy`` must be
installed, with a minimum ``numpy`` version of 1.19.3: ::

    [options]
    ...
    install_requires
        tkinter
        numpy>=1.19.3

Use of the more complex ``requirements.txt`` Python dependency mechanism is not
recommended, as it implies more control over the Python environment than makes
sense for a FontForge plugin.

When possible you should avoid dependencies on Python packages that require
compilation, either because they are written in another language or because
they rely on "external" libraries. The Windows version of FontForge has an
embedded Python environment that is not capable of compiling packages, so
(at least for now) Windows users will not be able to use your plugin if it
has such dependencies.

The Configuration System
^^^^^^^^^^^^^^^^^^^^^^^^

The plugin API offers an optional convention for setting and storing plugin-
specific parameters. It has two parts:

1. The initializing call to ``fontforge_plugin_init`` is passed a keyword
   parameter ``preferences_path``. This is a filesystem path that points to
   an appropriate location for storing plugin data. It is recommended that
   plugins either add an appropriate extension to the path (if they only
   need a single file) or to create a directory of that name and store
   files within it. The content should point to the same directory on every
   initialization unless the name of the plugin is changed.

   The parameter can be extracted from the ``**kw`` dictionary (see above)
   or caught explicitly by changing the definition to
   ``def fontforge_plugin_init(preferences_path=None, **kw):``
2. Plugin configuration is triggered by a call to ``fontforge_plugin_config``,
   which is a function defined on the same module or object as
   ``fontforge_plugin_init``. If a plugin defines that function it will be
   called when a user presses the "Configure" button in the Plugin dialog.
   If ``fontforge_plugin_init`` is *not* defined the button will be disabled.

Beyond this API it is entirely up to the plugin to store, retrieve, and offer
configuration choices to the user. Until FontForge provides more dialog choices
the latter may be difficult without resorting to ``tkinter``. It is also possible
to support file-based configuration and use an :py:meth:`fontforge.openFilename()`
dialog to ask for the file so that it can be copied into the appropriate location.
