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
AC_INIT([fontforge],[2.0.0_beta1],[fontforge-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([Unicode/ArabicForms.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_TESTDIR([tests],[fontforge])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CONFIG_HEADERS([inc/config.h])
AM_INIT_AUTOMAKE([subdir-objects foreign -Wall])
AM_MAINTAINER_MODE([enable])
AC_USE_SYSTEM_EXTENSIONS

# I am going to try enabling silent-rules by default, so builders are
# more likely to see warnings. Use --disable-silent-rules or "make
# V=1" if you need to see the rules.
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

m4_ifdef([AM_PROG_AR],[AM_PROG_AR])
LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
LTDL_INIT([nonrecursive])
FONTFORGE_CHECK_LTDL_VERSION

AC_PATH_XTRA
i_do_have_x="${have_x}"
i_do_have_gui="${i_do_have_x}"

AC_SUBST([HOST],["$host"])
AC_SUBST([MACAPP])

FONTFORGE_PLATFORM_SPECIFICS

VERSION_MAJOR=`AS_ECHO(["${PACKAGE_VERSION}"]) | sed -e 's/^\([[0-9]][[0-9]]*\).*/\1/'`
VERSION_MINOR=`AS_ECHO(["${PACKAGE_VERSION}"]) | sed -e 's/^[[0-9]][[0-9]]*\.\([[0-9]][[0-9]]*\).*/\1/'`
VERSION_PATCH=`AS_ECHO(["${PACKAGE_VERSION}"]) | sed -e 's/^[[0-9]][[0-9]]*\.[[0-9]][[0-9]]*\.\([[0-9]][[0-9]]*\).*/\1/'`
AC_SUBST([VERSION_MAJOR])
AC_SUBST([VERSION_MINOR])
AC_SUBST([VERSION_PATCH])

#--------------------------------------------------------------------------
#
# URLs for dependencies.

cairo_url="http://www.cairographics.org/"
freetype_url="http://www.freetype.org/"
giflib_url="http://giflib.sourceforge.net/"
libpng_url="http://www.libpng.org/"
libxml_url="http://www.xmlsoft.org/"
libspiro_url="http://libspiro.sourceforge.net/"
libunicodenames_url="https://github.com/chemoelectric/libunicodenames"

# Point to the Wikipedia page, primarily because one may want
# libjpeg-turbo rather than the reference implementation.
libjpeg_url="http://en.wikipedia.org/wiki/Libjpeg"

# Point to the Wikipedia page, so people can learn not to be fooled by
# the old website, which some while ago was hijacked by androids from
# beyond the orbit of Neptune.
libtiff_url="http://en.wikipedia.org/wiki/Libtiff"

#--------------------------------------------------------------------------
#
# Checks for programs.

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_GREP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_SED
PKG_PROG_PKG_CONFIG
AC_PATH_PROG([MSGFMT],[msgfmt],[msgfmt])
AC_PATH_PROG([XGETTEXT],[xgettext],[xgettext])
AC_PATH_PROG([CPROTO],[cproto],[cproto])
AC_PATH_PROG([INDENT],[indent],[indent])
AC_PATH_PROG([UNZIP],[unzip],[unzip])
AC_PATH_PROG([UPDATE_MIME_DATABASE],[update-mime-database],[update-mime-database])
AC_PATH_PROG([UPDATE_DESKTOP_DATABASE],[update-desktop-database],[update-desktop-database])
AC_PATH_PROG([PLUTIL],[plutil],[:])

FONTFORGE_ARG_ENABLE_REAL

AC_ARG_ENABLE([programs],
        [AS_HELP_STRING([--disable-programs],[do not build "fontforge" and related programs
                                              (but do build libraries and possibly the Python extensions)])],
        [i_do_have_programs="${enableval}"],
        [i_do_have_programs=yes])
AM_CONDITIONAL([THE_PROGRAMS],[test x"${i_do_have_programs}" = xyes])
AC_SUBST([THE_PROGRAMS],["${i_do_have_programs}"])

FONTFORGE_ARG_DISABLE([native-scripting],
        [AS_HELP_STRING([--disable-native-scripting],[disable the legacy fontforge scripting language])],
        [_NO_FFSCRIPT])
AM_CONDITIONAL([NATIVE_SCRIPTING],[test x"${i_do_have_native_scripting}" = xyes])

FONTFORGE_ARG_DISABLE_PYTHON_SCRIPTING
FONTFORGE_ARG_DISABLE_PYTHON_EXTENSION

AC_ARG_ENABLE([freetype-debugger],
        [AS_HELP_STRING([--enable-freetype-debugger[[=DIR]]],
                [use freetype's internal debugger within fontforge; requires source code of the
                 freetype library with which fontforge will be linked.
                 You need to set DIR to the top directory of the freetype sources,
                 or alternatively set the environment variable FREETYPE_SOURCE (see below)])],
        [i_do_have_freetype_debugger="${enableval}"],
        [AC_ARG_WITH([freetype-source],
                [AS_HELP_STRING([--with-freetype-source[[=DIR]]],[synonym for --enable-freetype-debugger])],
                [i_do_have_freetype_debugger="${withval}"],
                [i_do_have_freetype_debugger=no])])

FONTFORGE_ARG_ENABLE([capslock-for-alt],
        [AS_HELP_STRING([--enable-capslock-for-alt],
                [use capslock rather than alt/meta to alter behavior of tools in outline/bitmap windows])],
        [FONTFORGE_CONFIG_CAPSLOCK_FOR_ALT])

FONTFORGE_ARG_ENABLE([debug-raw-points],
        [AS_HELP_STRING([--enable-debug-raw-points],
                [add a raw mode to the points window of the debugger])],
        [FONTFORGE_CONFIG_SHOW_RAW_POINTS])

FONTFORGE_ARG_ENABLE([tile-path],
        [AS_HELP_STRING([--enable-tile-path],
                [enable a 'tile path' command (a variant of 'expand stroke')])],
        [FONTFORGE_CONFIG_TILEPATH])

FONTFORGE_ARG_ENABLE([gb12345],
        [AS_HELP_STRING([--enable-gb12345],
                [build the GB12345 encoding plugin])],
        [FONTFORGE_CONFIG_GB12345])
AM_CONDITIONAL([PLUGIN_GB12345],[test x"${i_do_have_gb12345}" = xyes])

FONTFORGE_ARG_ENABLE([maintainer-tools],
        [AS_HELP_STRING([--enable-maintainer-tools],
                [build programs used normally only by fontforge maintainers and developers])],
        [MAINTAINER_TOOLS])
AM_CONDITIONAL([MAINTAINER_TOOLS],[test x"${i_do_have_maintainer_tools}" = xyes])
AC_SUBST([MAINTAINER_TOOLS],["${i_do_have_maintainer_tools}"])

# Enable this experimental feature via an obscure cache variable
# rather than --enable.
AC_CACHE_CHECK([whether to enable native callbacks (experimental)],
               [fontforge_cv_native_callbacks],
               [if test x"${fontforge_cv_native_callbacks}" != yes; then
                   fontforge_cv_native_callbacks=no
                fi])
i_do_have_native_callbacks="${fontforge_cv_native_callbacks}"


#--------------------------------------------------------------------------
#
# Checks for libraries.

AC_SEARCH_LIBS([cos],[m])

dnl FIXME: Is this needed, given we are using libltdl?
dnl AC_SEARCH_LIBS([dlopen],[dl])

FONTFORGE_CONFIG_X_LIBRARIES

# zlib is a requirement. It is too commonly available to bother
# leaving out.
PKG_CHECK_MODULES([ZLIB],[zlib])

if test x"${i_do_have_gui}" = xyes; then
   PKG_CHECK_MODULES([PANGO],[pango >= 1.10 pangoxft])
fi

FONTFORGE_ARG_WITH_CAIRO
FONTFORGE_ARG_WITH_FREETYPE
AC_ARG_VAR([FREETYPE_SOURCE],
        [directory where freetype source code is located for --enable-freetype-debugger;
         however, if --enable-freetype-debugger=DIR is specified, then DIR overrides
         the setting of FREETYPE_SOURCE])
FONTFORGE_ARG_WITH_GIFLIB
FONTFORGE_ARG_WITH_LIBJPEG
FONTFORGE_ARG_WITH_LIBPNG
FONTFORGE_ARG_WITH_LIBTIFF
FONTFORGE_ARG_WITH_LIBXML
FONTFORGE_ARG_WITH_LIBSPIRO
FONTFORGE_ARG_WITH_LIBUNICODENAMES
FONTFORGE_ARG_WITH_ICONV

if test x"${i_do_have_cairo}" = xyes; then
   PKG_CHECK_MODULES([PANGOCAIRO],[pangocairo])
fi

FONTFORGE_CONFIG_FREETYPE_DEBUGGER

#--------------------------------------------------------------------------
#
# Checks for header files.

AC_CHECK_HEADERS([arpa/inet.h fcntl.h ieeefp.h inttypes.h])
AC_CHECK_HEADERS([langinfo.h libintl.h limits.h locale.h])
AC_CHECK_HEADERS([memory.h netdb.h netinet/in.h pthread.h stddef.h stdint.h stdlib.h])
AC_CHECK_HEADERS([string.h strings.h sys/ioctl.h sys/socket.h sys/time.h unistd.h wchar.h])
if test x"${i_do_want_iconv}" = xyes; then
   AC_CHECK_HEADERS([iconv.h])
fi

#--------------------------------------------------------------------------
#
# Checks for typedefs, structures, and compiler characteristics.

AC_C_BIGENDIAN
AC_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_TYPE_LONG_LONG_INT
if test x"${ac_cv_type_long_long_int}" = xyes; then
   AC_DEFINE([_HAS_LONGLONG],1,[Define if using 'long long int'.])
fi

AX_PTHREAD

FONTFORGE_COMPILER_FLAGS([WARNING_CFLAGS],
        [-Wunused -Wreturn-type
         -Wparentheses -Wformat -Wchar-subscripts
         -Wno-unused-but-set-variable -Wno-unused-result
         -Werror=missing-prototypes
         -Werror=implicit])

#--------------------------------------------------------------------------
# Checks for library functions.

AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_FUNC_MMAP
if test x"${ac_cv_func_mmap_fixed_mapped}" != xyes; then
   AC_DEFINE([_NO_MMAP],1,[Define if not using mmap.])
fi
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_FUNC_STRTOD
AC_CHECK_FUNCS([alarm atexit dup2 endpwent floor])
AC_CHECK_FUNCS([getcwd gethostbyname gettimeofday])
AC_CHECK_FUNCS([localeconv memmove memset mkdir munmap])
AC_CHECK_FUNCS([nl_langinfo pow putenv rint rmdir])
AC_CHECK_FUNCS([select setenv setlocale snprintf socket sqrt])
AC_CHECK_FUNCS([strcasecmp strchr strdup strerror])
AC_CHECK_FUNCS([strncasecmp strpbrk strrchr strstr])
AC_CHECK_FUNCS([strtol strtoul tzset uname])
AC_CHECK_FUNC([snprintf],[:],[AC_DEFINE([_NO_SNPRINTF],1,[Define if not using snprintf.])])
AC_CHECK_FUNC([tzset],[:],[AC_DEFINE([_NO_TZSET],1,[Define if not using tzset.])])

#--------------------------------------------------------------------------

AM_CONDITIONAL([GRAPHICAL_USER_INTERFACE],[test x"${i_do_have_gui}" = xyes])
AM_CONDITIONAL([MACINTOSH],[test x"${gww_ismac}" = xyes])

# The following conditionals are always false, so the directories
# never get built by default. Nevertheless, they will be included by
# "make dist".
AM_CONDITIONAL([DESKTOP],[test x != x])
AM_CONDITIONAL([FONTTOOLS],[test x != x])

FONTFORGE_SET_MY_CFLAGS
FONTFORGE_SET_MY_LIBS
FONTFORGE_CREATE_MAKEFILES
FONTFORGE_CREATE_PKGCONFIG_FILES
FONTFORGE_CREATE_SYSTEMSPECIFIC
FONTFORGE_CONFIG_TESTSUITE

AC_OUTPUT

AC_MSG_NOTICE([

Summary of optional features:

  real (floating pt) ${my_real_type}
  programs           ${i_do_have_programs}
  native scripting   ${i_do_have_native_scripting}
  python scripting   ${i_do_have_python_scripting}
  python extension   ${i_do_have_python_extension}
  freetype debugger  ${i_do_have_freetype_debugger}
  capslock for alt   ${i_do_have_capslock_for_alt}
  raw points mode    ${i_do_have_capslock_for_alt}
  tile path          ${i_do_have_tile_path}
  gb12345 encoding   ${i_do_have_gb12345}

Summary of optional dependencies:

  cairo              ${i_do_have_cairo}  	${cairo_url}
  freetype           ${i_do_have_freetype}  	${freetype_url}
  giflib             ${i_do_have_giflib}  	${giflib_url}
  libjpeg            ${i_do_have_libjpeg}  	${libjpeg_url}
  libpng             ${i_do_have_libpng}  	${libpng_url}
  libtiff            ${i_do_have_libtiff}  	${libtiff_url}
  libxml             ${i_do_have_libxml}  	${libxml_url}
  libspiro           ${i_do_have_libspiro}  	${libspiro_url}
  libunicodenames    ${i_do_have_libunicodenames}  	${libunicodenames_url}
  X Window System    ${i_do_have_x}
])

#--------------------------------------------------------------------------
