#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([fontforge-python-extension],[9999-experimental],[fontforge-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([fontforgepyhook.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([1.11.1 foreign silent-rules -Wall -Werror])
AM_MAINTAINER_MODE([enable])
AC_USE_SYSTEM_EXTENSIONS

LT_INIT([dlopen])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_OBJC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_SED
AM_PATH_PYTHON([2.3])
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_MODULES([PYTHON],[python-"${PYTHON_VERSION}"])
PKG_CHECK_MODULES([LIBFONTFORGE],[libfontforge])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

