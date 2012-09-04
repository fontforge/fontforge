dnl -*- autoconf -*-

dnl FONTFORGE_CREATE_PKGCONFIG_FILES
dnl --------------------------------
AC_DEFUN([FONTFORGE_CREATE_PKGCONFIG_FILES],
[
# FIXME: Make these take account of dependencies, at least to aid
# building programs with static libraries, in the absence of libtool
# archive files.
AC_CONFIG_FILES([fontforge.pc])
AC_CONFIG_FILES([libfontforge.pc])
AC_CONFIG_FILES([libfontforgeexe.pc])
])
