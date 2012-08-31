#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([fontforge-python-extension],[2.0-alpha1],[fontforge-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([fontforgepyhook.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([1.12.2 foreign -Wall -Werror])
AM_MAINTAINER_MODE([enable])
AC_USE_SYSTEM_EXTENSIONS

# I am going to try enabling silent-rules by default, so builders are
# more likely to see warnings. Use --disable-silent-rules or "make
# V=1" if you need to see the rules.
AM_SILENT_RULES([yes])

AM_PROG_AR
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
for i in -Wmissing-prototypes -Wunused -Wimplicit -Wreturn-type \
    -Wparentheses -Wformat -Wchar-subscripts -Wno-unused-but-set-variable; do
    AX_CHECK_COMPILE_FLAG([${i}],[CFLAGS="${i} ${CFLAGS}"])
done

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

