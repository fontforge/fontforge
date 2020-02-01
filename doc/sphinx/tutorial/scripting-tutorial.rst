FontForge Scripting Tutorial
============================

.. note::

  FontForge now provides python scripting. If you are familiar with python
  that is probably a better choice. There is a lot of information available on
  `python <http://www.python.org/doc/>`__, I shan't repeat it. FontForge's own
  additions to python are documented :doc:`here </scripting/python>`.

I try to keep things at a fairly elementary level, but this is *not* an attempt
to teach programming.


A simple example
----------------

Suppose you have a Type1 PostScript font (a pfb/afm combination) and you would
like to convert it into a TrueType font. What would a script look like that
could do this?

If you were doing this with the UI you would first
:ref:`File->Open <filemenu.Open>` the font and then
:ref:`File->Generate <filemenu.Generate>` a truetype font. You do essentially
the same thing when writing a script:


Initial Solution
^^^^^^^^^^^^^^^^

::

   Open($1)
   Generate($1:r + ".ttf")

There is usually a scripting function with the same name as a menu command
(well, the same as the English name of the menu command).

':ref:`$1 <scripting.variables>`' is magic. It means the
:ref:`first argument passed to the script <scripting-tutorial.Invoking>`.

':ref:`$1:r + ".ttf" <scripting.Expressions>` ' is even more complicated magic.
It means: 'take the first argument ($1) and remove the extension (which is
probably ".pfb") and then append the string ".ttf" to the filename.'

The Generate scripting command decides what type of font to generate depending
on the extension of the filename you give it. Here we give it an extension of
"ttf" which means truetype.

Note that I make no attempt to load an afm file. That's because the Open command
will do this automagically if it is in the same directory as the pfb.


Real World Considerations
^^^^^^^^^^^^^^^^^^^^^^^^^

So that's what the script looks like. To be useful it should probably live in a
file of its own. So create a file called "convert.pe" and store the above script
in it.

But to be even more useful you should add a comment line to the beginning of the
script (a comment line is one that starts with the '#' character:

::

   #!/usr/local/bin/fontforge
   Open($1)
   Generate($1:r + ".ttf")

Having done that type:

::

   $ chmod +x convert.pe

This comment is not important to fontforge, but it is meaningful to the unix
shell, as we will see in the next section.


.. _scripting-tutorial.Invoking:

Invoking a script and passing it arguments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Ok, now we've got basic script. How do we use it?

Well we can pass it to FontForge directly by typing

::

   $ fontforge -script convert.pe foo.pfb

But if you added the comment above you can also type:

::

   $ convert.pe foo.pfb

And the shell knows to call fontforge to process the script.


Using loops
^^^^^^^^^^^

That's all well and good, but if you have lots of fonts to convert this might
get tedious. So let's change our script so it will take lots of filenames which
we can then process one at a time.

::

   #!/usr/local/bin/fontforge
   i=1
   while ( i<$argc )
     Open($argv[i])
     Generate($argv[i]:r + ".ttf")
     i = i+1
   endloop

Here we have introduced the variables ``$argc`` and ``$argv``. The first is
simple the number of arguments passed to this script, while the second is an
array containing all those arguments, and ``$argv[i]`` means the i'th argument
passed.

Then we have:

::

   i=1

This declares that we have a local variable called "i" and assigns it the value
1.

The while loop will execute all statements between the "``while``" keyword and
the "``endloop``" keyword as long as the condition ``( i<$argv )`` is true. In
other words as long as there are more arguments to convert the loop will keep
going.

And we can invoke this script with

::

   $ convert.pe *.pfb

Or something similar.


Complexities
^^^^^^^^^^^^

Now suppose that you wanted a script that could convert a truetype font to an
opentype font as well as a type1 font to a truetype. Well let's make our script
even more complex:

::

   #!/usr/local/bin/fontforge
   i=1
   format=".ttf"
   while ( i<$argc )
     if ( $argv[i]=="-format" || $argv[i]=="--format" )
       i=i+1
       format = $argv[i]
     else
       Open($argv[i])
       Generate($argv[i]:r + format)
     endif
     i = i+1
   endloop

And this could be invoked with something like:

::

   $ convert.pe --format ".ttf" *.pfb --format ".otf" *.ttf

So now we have a new variable, ``format``, which contains the type of output we
want to use from now on. We initialize it to truetype, ".ttf", but if the user
gives us an argument called "--format" (or "-format") then we change the output
to be whatever the user asked for.

We've introduced the "``if``" statement here. This statement will execute the
statements between "``if``" and "``else``" if the condition
``( $argv[i]=="-format" || $argv[i]=="--format" )`` is true, otherwise it will
execute the statements between "``else``" and "``endif``". The || operator means
"or", so the condition is true if $argv[i] is either "-format" or "--format".

We really should do some error checking to make sure:

* There was another argument to store into the ``format`` variable
* The argument contained a reasonable value (.ttf, .pfb, .otf, .svg, ...)

::

   #!/usr/local/bin/fontforge
   i=1
   format=".ttf"
   while ( i<$argc )
     if ( $argv[i]=="-format" || $argv[i]=="--format" )
       i=i+1
       if ( i<$argc )
         format = $argv[i]
         if ( format!=".ttf" && format!=".otf" && \
             format!=".pfb" && format!=".svg" )
           Error( "Expected one of '.ttf', '.otf', '.pfb' or '.svg' for format" )
         endif
       endif
     else
       Open($argv[i])
       Generate($argv[i]:r + format)
     endif
     i = i+1
   endloop

Note that when we had a long line we broke it in two by using a backslash.
Normally the end of a line marks the end of a statement, so we need the
backslash to indicate the statement continues onto the next line.

Now that will produce a valid postscript font from a truetype one if we want...
But we can improve on that conversion:

::

   #!/usr/local/bin/fontforge
   i=1
   format=".ttf"
   while ( i<$argc )
     if ( $argv[i]=="-format" || $argv[i]=="--format" )
       i=i+1
       if ( i<$argc )
         format = $argv[i]
         if ( format!=".ttf" && format!=".otf" && \
             format!=".pfb" && format!=".svg" )
           Error( "Expected one of '.ttf', '.otf', '.pfb' or '.svg' for format" )
         endif
       endif
     else
       Open($argv[i])
       if ( $order==2 && (format==".otf" || format==".pfb" ))
         SetFontOrder(3)
         SelectAll()
         Simplify(128+32+8,1.5)
         ScaleToEm(1000)
       elseif ( $order==3 && format==".ttf" )
         ScaleToEm(2048)
         RoundToInt()
       endif
       Generate($argv[i]:r + format)
     endif
     i = i+1
   endloop

By its nature a truetype font will contain more points than will a postscript
font, but we can use the Simplify command to reduce that number when we convert
from one format to another. Also PostScript fonts should have 1000 units to the
em while TrueType fonts should have a power of 2 units/em (generally 2048 or
1024), so enforce these conventions. Finally TrueType fonts only support
integral (or in some cases half-integral) coordinates for points.


Other Examples
--------------


Adding Accented Characters
^^^^^^^^^^^^^^^^^^^^^^^^^^

Very few Type1 fonts have the full unicode range of accented characters. With
FontForge it is fairly easy to load a Type1 font, add as many possible accented
characters as the font permits (If the font does not contain ogonek, then FF
won't be able to create Aogonek).

::

   #!/usr/local/bin/fontforge
   Open($1)
   Reencode("unicode")
   SelectWorthOutputting()
   SelectInvert()
   BuildAccented()
   Generate($1:r + ".otf")


Merging a type1 and type1 expert font and creating appropriate GSUB tables.
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Adobe used to ship font packs containing a normal font and an "expert" font
which included small caps, lower case numbers, etc. Now-a-days that should all
be stuffed into one otf font with appropriate GSUB entries linking the glyphs.

::

   #!/usr/local/bin/fontforge
   Open($1)
   MergeFonts($2)
   RenameGlyphs("AGL with PUA")
   SelectAll()
   DefaultATT("*")


More examples
^^^^^^^^^^^^^

See the :ref:`page on scripting <scripting.Example>`.