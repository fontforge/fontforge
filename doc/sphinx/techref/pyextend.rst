Extending FontForge With Python
===============================

FontForge includes an extensive :doc:`Python API </scripting/python>` which is
most often used in scripts to alter and generate font files. However, this
API also provides ways of extending the functionality of the FontForge program,
including the user interface. Most of these are documented in the :ref:`User Interface Module Functions <fontforge.ui_functions>` section of the Python documentation.

This document will gradually be expanded with advice on using those functions
together with other parts of the API to build custom tools. For now, though,
it documents the FontForge plugin API.

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
        return len(getSelectedPoints(glyph.foreground))==2

    def addMidContour(u, glyph):
        l = glyph.foreground
        pl = getSelectedPoints(l)
        if len(pl) != 2:
            fontforge.postError("Bad selection", "You must select "
            "exactly two on-curve points to add a midpoint contour.")
            return
        nc = fontforge.contour()
        nc.insertPoint(((pl[0].x+pl[1].x)/2, (pl[0].y+pl[1].y)/2, True,
                fontforge.splineCorner, True))
        l += nc
        glyph.preserveLayerAsUndo(1)
        glyph.foreground = l

    fontforge.registerMenuItem(addMidContour, midContourEnable,
            None, ("Glyph"), None, "Add Midpoint Contour")

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
        fontforge.registerMenuItem(addMidContour, midContourEnable,
                None, ("Glyph"), None, "Add Midpoint Contour")

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
``setup.py`` file). It should have contents analgous to these: ::

    [metadata]
    name = fontforge-midcontour-author
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

The ``name`` field is the name of the package, and by convention it should
end with the developers PyPI username. The ``url`` should point to your
project page or a GitHub (or other such service) repository for the
plugin code.

Obviously the ``description`` field should contain a one-line description
of the plugin. However, it is important that this description (or, it is
not possible, the README file) contain the string "FontForge_plugin".
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

The token on the left of the indented line is the plugin *name*. This will
appear in the "Configure Plugins..." dialog and other contexts. It will
normally be the name of your package except without "fontforge" and with
optional capitalization. It can contain the usual alphanumerics plus
underscores but also spaces (although spaces are not recommended).

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
