Files used by FontForge
=======================

.. object:: ~/.FontForge

   FontForge creates this directory the first time it starts up. It contains
   various files FontForge uses to hold context across invocations.

--------------------------------------------------------------------------------

   .. object:: ~/.FontForge/prefs

      This file contains user preference settings.

   .. object:: ~/.FontForge/groups

      This file contains groups as defined by
      :menuselection:`Encoding --> Define Groups...`

   .. object:: ~/.FontForge/Encodings.ps

      If there are any user defined encodings they reside in this file.

   .. object:: ~/.FontForge/autosave

      This directory contains the files used to drive the crash-recovery
      process. When FontForge is running there will be one file in this
      directory corresponding to each font loaded into FontForge that has been
      modified. These files will contain all changed glyphs.

      When FontForge exits normally, this directory will be cleaned up and there
      should be no files in it.

   .. object:: ~/.FontForge/python

      This directory contains any python init scripts you want to run when
      FontForge starts

.. object:: /usr/local/share (or $PREFIX/share)
            /usr/local/share/locale/??/LC_MESSAGES/FontForge.mo

   Contain translations of the ui for various locales. (used after November
   2005)

.. object:: /usr/local/share/fontforge/*.cidmap

   "Encoding" files for Adobe's cid formats.
   `You can pull down a compressed archive from here. <https://fontforge.org/cidmaps.tar.gz>`_

.. object:: /usr/local/share/fontforge/python

   This directory contains any python init scripts you want available to
   everyone on your system.

.. object:: /usr/local/share/doc/fontforge/*.html

   Optional location for online documentation.
