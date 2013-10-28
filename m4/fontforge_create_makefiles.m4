dnl -*- autoconf -*-

dnl FONTFORGE_CREATE_MAKEFILES
dnl --------------------------
AC_DEFUN([FONTFORGE_CREATE_MAKEFILES],
[
# Portable makefiles.
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([inc/Makefile])
AC_CONFIG_FILES([libltdl/Makefile])
AC_CONFIG_FILES([Unicode/Makefile])
AC_CONFIG_FILES([gutils/Makefile])
AC_CONFIG_FILES([collab/Makefile])
AC_CONFIG_FILES([hotkeys/Makefile])
AC_CONFIG_FILES([share/Makefile])
AC_CONFIG_FILES([gdraw/Makefile])
AC_CONFIG_FILES([fontforge/Makefile])
AC_CONFIG_FILES([fontforge/pixmaps/Makefile])
AC_CONFIG_FILES([fontforge/pixmaps/2012/Makefile])
AC_CONFIG_FILES([fontforge/pixmaps/tango/Makefile])
AC_CONFIG_FILES([fontforge/collab/Makefile])
AC_CONFIG_FILES([po/Makefile])
AC_CONFIG_FILES([mackeys/Makefile])
AC_CONFIG_FILES([htdocs/Makefile])
AC_CONFIG_FILES([pycontrib/Makefile])
AC_CONFIG_FILES([pycontrib/simple/Makefile])
AC_CONFIG_FILES([pycontrib/collab/Makefile])
AC_CONFIG_FILES([nodejs/Makefile])
AC_CONFIG_FILES([nodejs/collabwebview/Makefile])
AC_CONFIG_FILES([nodejs/collabwebview/css/Makefile])
AC_CONFIG_FILES([nodejs/collabwebview/js/Makefile])
AC_CONFIG_FILES([nodejs/collabwebview/js/contentEditable/Makefile])
AC_CONFIG_FILES([pyhook/Makefile])
AC_CONFIG_FILES([cidmap/Makefile])
AC_CONFIG_FILES([plugins/Makefile])
AC_CONFIG_FILES([tests/Makefile])
AC_CONFIG_FILES([desktop/Makefile])
AC_CONFIG_FILES([fonttools/Makefile])
AC_CONFIG_FILES([osx/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/Resources/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/Resources/English.lproj/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/MacOS/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Inputbox.nib/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/MainMenu.nib/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Msgbox.nib/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/PopUpButton.nib/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Progressbar.nib/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/SecureInputbox.nib/Makefile])
AC_CONFIG_FILES([osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Textbox.nib/Makefile])


# GNUmakefiles that act as wrappers around portable makefiles and
# provide additional targets.
AC_CONFIG_FILES([GNUmakefile])
AC_CONFIG_FILES([fontforge/GNUmakefile])
AC_CONFIG_FILES([po/GNUmakefile])

# GNUmakefile snippets for use with "include".
AC_CONFIG_FILES([mk/xgettext_search.mk])
])
