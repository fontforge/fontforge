dnl -*- autoconf -*-

dnl FONTFORGE_CREATE_MAKEFILES
dnl --------------------------
AC_DEFUN([FONTFORGE_CREATE_MAKEFILES],
[
# Portable makefiles.
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([inc/Makefile])
AC_CONFIG_FILES([pluginloading/Makefile])
AC_CONFIG_FILES([Unicode/Makefile])
AC_CONFIG_FILES([gutils/Makefile])
AC_CONFIG_FILES([gdraw/Makefile])
AC_CONFIG_FILES([fontforge/Makefile])
AC_CONFIG_FILES([fontforge/pixmaps/Makefile])
AC_CONFIG_FILES([po/Makefile])
AC_CONFIG_FILES([po/mackeys/Makefile])
AC_CONFIG_FILES([htdocs/Makefile])
AC_CONFIG_FILES([pycontrib/Makefile])
AC_CONFIG_FILES([plugins/Makefile])
AC_CONFIG_FILES([tests/Makefile])

# GNUmakefiles that act as wrappers around portable makefiles, to do
# fancy things that are mainly for the benefit of maintainers.
AC_CONFIG_FILES([fontforge/GNUmakefile])
])
