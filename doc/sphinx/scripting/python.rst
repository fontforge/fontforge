Python Scripting
================

.. default-domain:: py

I assume you have a working knowledge of `Python <http://www.python.org/doc/>`_.
FontForge implements two Python modules -- one great huge one called :py:mod:`fontforge`
which provides access to as much of FontForge's functionality as I've had
time to write, and one tiny one called :py:mod:`psMat` which provides quick
access to some useful transformations expressed as PostScript matrices.

In python terms fontforge *embeds* python. It is possible to build
fontforge so that it is also a :ref:`python extension <python.extension>`.

FontForge Modules
-----------------

.. toctree::
   :maxdepth: 3
   :glob:

   python/*

Command line convenience
------------------------
For convenience, Python commands given as a ``-c`` argument on the
command line have the following code prepended:

::

   from sys import argv; from fontforge import *

Hence, the trivial script to convert a font can be written:

::

   fontforge -c 'open(argv[1]).generate(argv[2])'


Trivial example
---------------
::

   import fontforge                                 #Load the module
   amb=fontforge.open("Ambrosia.sfd")               #Open a font
   amb.selection.select(("ranges",None),"A","Z")    #select A-Z
   amb.copy()                                       #Copy those glyphs into the clipboard

   n=fontforge.font()                               #Create a new font
   n.selection.select(("ranges",None),"A","Z")      #select A-Z of it
   n.paste()                                        #paste the glyphs above in
   print n["A"].foreground                          #test to see that something
                                                    #  actually got pasted
   n.fontname="NewFont"                             #Give the new font a name
   n.save("NewFont.sfd")                            #and save it.


.. _python.extension:

FontForge as a Python extension
-------------------------------

FontForge's Python modules can be used independently of the GUI application.

Installation via pip
^^^^^^^^^^^^^^^^^^^^

::

   pip install fontforge

Pre-built wheels are available for:

- Linux: x86_64, aarch64
- macOS: x86_64, arm64
- Windows: x86_64

The wheels include all required native libraries (FreeType, libxml2, etc.).

Installation from system packages
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When FontForge is installed via system package manager or built from source
with ``-DENABLE_PYTHON_EXTENSION=ON``, the modules are installed to the
system Python's site-packages.

Usage
^^^^^

::

   >>> import fontforge
   >>> import psMat

For technical details on building wheels and the extension architecture,
see :doc:`python-extension`.
