#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Copyright (C) 2000-2012 by George Williams
# Copyright (C) 2012 by Barry Schwartz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# The name of the author may not be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

AC_PREREQ([2.68])
AC_INIT([fontforge-python-extension],[2.0.0-beta1],[fontforge-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([fontforgepyhook.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AM_MAINTAINER_MODE([enable])
AC_USE_SYSTEM_EXTENSIONS

# I am going to try enabling silent-rules by default, so builders are
# more likely to see warnings. Use --disable-silent-rules or "make
# V=1" if you need to see the rules.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
LT_INIT([dlopen])

# Checks for programs.
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
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
    -Wparentheses -Wformat -Wchar-subscripts \
    -Wno-unused-but-set-variable -Wno-unused-result; do
    AX_CHECK_COMPILE_FLAG([${i}],[CFLAGS="${i} ${CFLAGS}"])
done

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

