#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Copyright (C) 2000-2012 by George Williams
# Copyright (C) 2012 by Barry Schwartz
# Copyright (C) 2013 by Ben Martin
# Copyright (C) 2014 by Reuben Thomas <rrt@sc3d.org>
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

#--------------------------------------------------------------------------
# PackageTimestamp version
m4_define([fontforge_package_stamp], [20170731])
#--------------------------------------------------------------------------
# Making point releases:
#   fontforge_major_version += 0;
#   fontforge_minor_version += 1; (patches or added function(s))
#
# If any new functions have been added:
#   fontforge_major_version += 0;
#   fontforge_minor_version += 1; (added function(s))
#
# If backwards compatibility has been broken:
#   fontforge_major_version += 1;
#   fontforge_minor_version = 0;
#
m4_define([fontforge_major_version], [2]) dnl 20120731-b was 1.0
m4_define([fontforge_minor_version], [0])
m4_define([libgdraw_major_version], [5]) dnl 20120731-b was 4.10
m4_define([libgdraw_minor_version], [0])
m4_define([libgioftp_major_version], [2]) dnl 20120731-b was 1.0
m4_define([libgioftp_minor_version], [0])
m4_define([libgunicode_major_version], [4]) dnl 20120731-b was 3.2
m4_define([libgunicode_minor_version], [0])
m4_define([libgutils_major_version], [2]) dnl 20120731-b was 1.3
m4_define([libgutils_minor_version], [0])
m4_define([libzmqcollab_major_version], [2]) dnl n/a
m4_define([libzmqcollab_minor_version], [0])
m4_define([libfontforgeexe_major_version], [fontforge_major_version]) dnl matches fontforge
m4_define([libfontforgeexe_minor_version], [fontforge_minor_version])
m4_define([libfontforge_major_version], [fontforge_major_version]) dnl matches fontforge
m4_define([libfontforge_minor_version], [fontforge_minor_version])

#--------------------------------------------------------------------------
# Setup variables before running AC_INIT()
m4_define([fontforge_version],
          [fontforge_major_version.fontforge_minor_version.fontforge_package_stamp])
m4_define([fontforge_info],
          [fontforge_major_version:fontforge_minor_version:0])
m4_define([fontforge_package_name],[fontforge])
m4_define([libfontforgeexe_info],[libfontforgeexe_major_version:libfontforgeexe_minor_version:0])
m4_define([libfontforge_info],[libfontforge_major_version:libfontforge_minor_version:0])
m4_define([libgdraw_info],[libgdraw_major_version:libgdraw_minor_version:0])
m4_define([libgioftp_info],[libgioftp_major_version:libgioftp_minor_version:0])
m4_define([libgunicode_info],[libgunicode_major_version:libgunicode_minor_version:0])
m4_define([libgutils_info],[libgutils_major_version:libgutils_minor_version:0])
m4_define([libzmqcollab_info],[libzmqcollab_major_version:libzmqcollab_minor_version:0])

#--------------------------------------------------------------------------

# We need this in order to pass things through automake when it doesn't like them.
BLANK1=''; AC_SUBST(BLANK1)
AC_SUBST([BLANK2],[""])

#--------------------------------------------------------------------------
AC_INIT([fontforge],[fontforge_version],[fontforge-devel@lists.sourceforge.net],
	[fontforge_package_name],[https://github.com/fontforge/fontforge])
AC_CONFIG_SRCDIR([Unicode/ArabicForms.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_TESTDIR([tests],[fontforge])
AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CONFIG_HEADERS([config.h inc/fontforge-config.h:config.h])
AM_INIT_AUTOMAKE([foreign -Wall subdir-objects tar-pax])
AM_MAINTAINER_MODE([enable])
AC_USE_SYSTEM_EXTENSIONS
AX_CODE_COVERAGE

#--------------------------------------------------------------------------
#
# Checks for programs.

AC_PROG_AWK
AC_PROG_CC
AC_PROG_CC_C99
AM_PROG_CC_C_O
gl_EARLY
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_GREP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_SED
AC_PATH_PROG([STRIP],[strip],[:])

# Check for Objective C compiler
m4_ifdef([AC_PROG_OBJC], [
    AC_PROG_OBJC
], [
    AM_CONDITIONAL([am__fastdepOBJC], false)
    AC_SUBST([OBJC], [$CC])
    AC_SUBST([OBJCFLAGS], [$CFLAGS])
])

#--------------------------------------------------------------------------
# automake 1.12 requires AM_PROG_AR but automake < 1.11.2 doesn't recognize it.
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

#--------------------------------------------------------------------------
# Enable silent build rules by default, this requires atleast Automake-1.11
# Disable by passing --disable-silent-rules to configure or using make V=1
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],[AC_SUBST([AM_DEFAULT_VERBOSITY],[1])])


LT_CONFIG_LTDL_DIR([libltdl])
LT_INIT([dlopen])
LTDL_INIT([recursive convenience])
FONTFORGE_CHECK_LTDL_VERSION

#--------------------------------------------------------------------------
# init these early so they can be filled by packages (especially glib-2.0).
my_cflags=
my_libs=

# The following is for the benefit of the libltdl Makefile,
# which expects to find <inc/fontforge-config.h> in top_builddir,
# and for some other things that use a path relative to top_srcdir.
CPPFLAGS="${CPPFLAGS} AS_ESCAPE([-I${top_builddir}]) AS_ESCAPE([-I${top_srcdir}])"


FONTFORGE_ARG_ENABLE_GDK

if test x"${fontforge_can_use_gdk}" != xyes; then
    AC_PATH_XTRA
    i_do_have_x="${have_x}"
fi

if test x"${i_do_have_x}" = xyes || test x"${fontforge_can_use_gdk}" = xyes; then
    i_do_have_gui=yes
fi

AC_SUBST([HOST],["$host"])
AC_SUBST([MACAPP])

FONTFORGE_PLATFORM_SPECIFICS

#--------------------------------------------------------------------------
# Pass version variables to fontforge-config.h
AC_DEFINE([FONTFORGE_VERSION_MAJOR],[fontforge_major_version],[FontForge Major Version])
AC_DEFINE([FONTFORGE_VERSION_MINOR],[fontforge_minor_version],[FontForge Minor Version])
AC_DEFINE([FONTFORGE_LIBFFE_VERSION_MAJOR],[libfontforgeexe_major_version],[ExeLibFontForge Major Version])
AC_DEFINE([FONTFORGE_LIBFFE_VERSION_MINOR],[libfontforgeexe_minor_version],[ExeLibFontForge Minor Version])
AC_DEFINE([FONTFORGE_LIBFF_VERSION_MAJOR],[libfontforge_major_version],[LibFontForge Major Version])
AC_DEFINE([FONTFORGE_LIBFF_VERSION_MINOR],[libfontforge_minor_version],[LibFontForge Minor Version])

#--------------------------------------------------------------------------
# Pass library version variables to several MAKEFILE.AM
AC_SUBST([FONTFORGE_VERSION],[fontforge_version])
AC_SUBST([LIBFONTFORGEEXE_VERSION],[libfontforgeexe_info])
AC_SUBST([LIBFONTFORGE_VERSION],[libfontforge_info])
AC_SUBST([LIBGDRAW_VERSION],[libgdraw_info])
AC_SUBST([LIBGIOFTP_VERSION],[libgioftp_info])
AC_SUBST([LIBGUNICODE_VERSION],[libgunicode_info])
AC_SUBST([LIBGUTILS_VERSION],[libgutils_info])
AC_SUBST([LIBZMQCOLLAB_VERSION],[libzmqcollab_info])

AC_SUBST([VERSION_MAJOR],[fontforge_major_version])
AC_SUBST([VERSION_MINOR],[fontforge_minor_version])
AC_SUBST([VERSION_PATCH],[fontforge_package_stamp])

#--------------------------------------------------------------------------
# URLs for dependencies.

cairo_url="http://www.cairographics.org/"
freetype_url="http://www.freetype.org/"
giflib_url="http://giflib.sourceforge.net/"
libpng_url="http://www.libpng.org/"
libxml_url="http://www.xmlsoft.org/"
libspiro_url="https://github.com/fontforge/libspiro"
libuninameslist_url="https://github.com/fontforge/libuninameslist"
libzmq_url="http://www.zeromq.org/"
libreadline_url="http://www.gnu.org/software/readline"
libxorg_url="http://www.x.org/"
python_url="https://www.python.org/"
gdk_url="http://www.gtk.org/"
woff2_url="https://github.com/google/woff2"

# Point to the Wikipedia page, primarily because one may want
# libjpeg-turbo rather than the reference implementation.
libjpeg_url="http://en.wikipedia.org/wiki/Libjpeg"

# Point to the Wikipedia page, so people can learn not to be fooled by
# the old website, which some while ago was hijacked by androids from
# beyond the orbit of Neptune.
libtiff_url="http://en.wikipedia.org/wiki/Libtiff"

#--------------------------------------------------------------------------

# Required for building FontForge package, plus we use PKG_CHECK_MODULES in
# a few checks and searches for programs and libraries.
# NOTE: Some distros don't have /usr/local included in the /etc/ld.so PATH,
# so, PKG_CHECK_MODULES may not find libraries you compile into /usr/local.
m4_ifndef([PKG_PROG_PKG_CONFIG],[m4_fatal(
  [Could not locate the pkg-config autoconf macros.
   These are usually located in /usr/share/aclocal/pkg.m4. If your macros
   are in a different location, try setting the environment variable
   ACLOCAL="aclocal -I/other/macro/dir" before running autoreconf.])])
PKG_PROG_PKG_CONFIG([0.25])

AC_PATH_PROG([MSGFMT],[msgfmt],[msgfmt])
AC_PATH_PROG([XGETTEXT],[xgettext],[xgettext])
AC_PATH_PROG([INDENT],[indent],[indent])
AC_PATH_PROG([WGET],[wget],[wget])
AC_PATH_PROG([UNZIP],[unzip],[unzip])
AC_PATH_PROG([UPDATE_MIME_DATABASE],[update-mime-database],[:])
AC_PATH_PROG([UPDATE_DESKTOP_DATABASE],[update-desktop-database],[:])
AC_PATH_PROG([PLUTIL],[plutil],[:])


dnl###############################
dnl###############################

AM_CONDITIONAL([XDG_DESKTOP],[test x"$UPDATE_MIME_DATABASE" != "x:" -a x"$UPDATE_DESKTOP_DATABASE" != "x:"])

FONTFORGE_ARG_ENABLE_REAL

AC_ARG_ENABLE([programs],
        [AS_HELP_STRING([--disable-programs],[do not build "fontforge" and related programs
                                              (but do build libraries and possibly the Python extensions)])],
        [i_do_have_programs="${enableval}"],
        [i_do_have_programs=yes])
AM_CONDITIONAL([THE_PROGRAMS],[test x"${i_do_have_programs}" = xyes])
AC_SUBST([THE_PROGRAMS],["${i_do_have_programs}"])

FONTFORGE_ARG_ENABLE([debug],
        [AS_HELP_STRING([--enable-debug],[Turn on debug build and FONTFORGE_DEBUG conditional code])],
        [FONTFORGE_DEBUG])
if test x$enable_debug = xyes ; then
    CFLAGS="$CFLAGS -g -O0"
    CPPFLAGS="$CPPFLAGS -g -O0"
fi

FONTFORGE_ARG_DISABLE([native-scripting],
        [AS_HELP_STRING([--disable-native-scripting],[disable the legacy fontforge scripting language])],
        [_NO_FFSCRIPT])
AM_CONDITIONAL([NATIVE_SCRIPTING],[test x"${i_do_have_native_scripting}" = xyes])

FONTFORGE_ARG_DISABLE_PYTHON_SCRIPTING_AND_EXTENSION

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
# ' make syntax checkers happy

FONTFORGE_ARG_ENABLE([debug-raw-points],
        [AS_HELP_STRING([--enable-debug-raw-points],
                [add a raw mode to the points window of the debugger])],
        [FONTFORGE_CONFIG_SHOW_RAW_POINTS])

FONTFORGE_ARG_ENABLE([tile-path],
        [AS_HELP_STRING([--enable-tile-path],
                [enable a 'tile path' command (a variant of 'expand stroke')])],
        [FONTFORGE_CONFIG_TILEPATH])

FONTFORGE_ARG_ENABLE([write-pfm],
        [AS_HELP_STRING([--enable-write-pfm],
                [add the ability to save a PFM file without creating the associated font file])],
        [FONTFORGE_CONFIG_WRITE_PFM])

#--------------------------------------------------------------------------
# Theme pixmap ICON selection.
# Go with tango by default. Retain older 2012 theme for those who like it.
themename=tango

FONTFORGE_ARG_ENABLE([theme-2012],
        [AS_HELP_STRING([--enable-theme-2012],
                [use old circa 2012 icons instead of using the tango default icons])],
        [FONTFORGE_THEME_TANGO])
if test x$enable_theme_2012 = xyes; then
   themename=2012
fi

# select the theme.
AM_CONDITIONAL([USE_THEME_2012],[test x"$themename" = x2012])
AM_CONDITIONAL([USE_THEME_TANGO],[test x"$themename" = xtango])

#--------------------------------------------------------------------------

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

#--------------------------------------------------------------------------
# Check for autotrace and potrace (don't do anything with this yet)
dnl check for autotrace (v0.31-1) commented-out due to bug in autotrace.m4
dnl which requires brackets around [AUTOTRACE] on line 7.
i_do_find_auto_po_trace="no"
dnl AC_PATH_PROG([AUTO_TRACE],[autotrace],[no])
AC_PATH_PROG([PO_TRACE],[potrace],[no])
if test x"${PO_TRACE}" != xno; then
    i_do_find_auto_po_trace="yes"
fi
dnl if test x"${AUTO_TRACE}" != xno; then
dnl     i_do_find_auto_po_trace="yes"
dnl fi

#--------------------------------------------------------------------------
#
# Checks for libraries.

# Check for math.h include and math library.
AC_CHECK_HEADER([math.h],
   AC_SEARCH_LIBS([cos],[m],[have_libm=yes],
      AC_CHECK_LIB([m],[cos],[have_libm=yes],
         AC_CHECK_FUNC([cos],[have_libm=yes]))))
if test x"${have_libm}" != xyes; then
    AC_MSG_FAILURE([ERROR: Please install Math library and math.h],[1])
fi

dnl FIXME: Is this needed, given we are using libltdl?
dnl The dlopen() function is in the C library for *BSD and in libdl on GLIBC-based systems
dnl AC_SEARCH_LIBS([dlopen], [dl dld])
AC_CHECK_HEADER([ltdl.h],,AC_MSG_FAILURE([ERROR: Please install ltdl.h developer files],[1]))

if test x"${fontforge_can_use_gdk}" != xyes; then
    FONTFORGE_CONFIG_X_LIBRARIES
fi

# zlib is a requirement. It is too commonly available to bother leaving out.
# Try package first, if failed then try library searches and a header check.
PKG_CHECK_MODULES([ZLIB],[zlib],[have_zlib=yes],[have_zlib=no])
if test x"${have_zlib}" != xyes; then
   AC_SEARCH_LIBS([inflate],[z zlib zdll],[have_zlib=yes],[
      AC_CHECK_LIB([z],[inflate],[have_zlib=yes],[
      AC_CHECK_LIB([zlib],[inflate],[have_zlib=yes],[
      AC_CHECK_LIB([zdll],[inflate],[have_zlib=yes])])])],)
   if test x"${have_zlib}" = xyes; then
      AC_CHECK_HEADER([zlib.h],[],[have_zlib=no])
   fi
fi
if test x"${have_zlib}" != xyes; then
   AC_MSG_FAILURE([ERROR: Please install ZLIB library and zlib.h include file],[1])
fi

# glib became a requirement, but is also required if using GDK GUI system.
# before checking for glib-2.0, we need to verify you have pthreads first.
# Different OSes might have pthreads, pthread, kthread, or something else.
have_pthreads=yes
AX_PTHREAD( ,[have_pthreads=no]) dnl avoid filling "yes" here
if test x"${have_pthreads}" = xyes; then
   case $host in
      *aix*) ;; dnl AIX uses separate threading compiler (see AX_PTHREAD).
      *) CC="$PTHREAD_CC" ;;
   esac
else
   # just check for a function and avoid searching for "-pthreads"
   AC_CHECK_FUNC([pthread_create],[have_pthreads=yes])
fi
AC_CHECK_HEADER([pthread.h],,[have_pthreads=no])
if test x"${have_pthreads}" != xyes; then
   AC_MSG_FAILURE([ERROR: Please install PTHREADS library with development include files],[1])
else
   AC_DEFINE(HAVE_PTHREAD_H,[1],[We have pthreads.h.])
fi
PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.6 gio-2.0],[have_glib=pkg],[have_glib=no])
# glib ships with gio library. Some systems have trouble accessing these.
# GLIB_CFLAGS='-pthread -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include'
# GLIB_LIBS='-lgio-2.0 -lgobject-2.0 -lglib-2.0'
if test x"${have_glib}" = xno; then
   PKG_CHECK_MODULES([GLIB],[glib-2.0 >= 2.6],[have_glib=pkg],[have_glib=no])
   PKG_CHECK_MODULES([GIO_UNIX],[gio-unix-2.0],[my_cflags="${my_cflags} ${GIO_UNIX_CFLAGS}" my_libs="${my_libs} ${GIO_UNIX_LIBS}"],[have_glib=no])
fi
if test x"${have_glib}" != xpkg; then
   AC_MSG_FAILURE([ERROR: Please install GLIB-2.0 (with GIO-2.0 or GIO-UNIX-2.0) libraries with development include files],[1])
fi

if test x"${i_do_have_x}" = xyes; then
   PKG_CHECK_MODULES([PANGO],[pango >= 1.10 pangoxft])
fi

PKG_CHECK_MODULES([FREETYPE],[freetype2 >= 2.3.7])
PKG_CHECK_MODULES([LIBXML],[libxml-2.0])

AC_ARG_VAR([FREETYPE_SOURCE],
        [directory where freetype source code is located for --enable-freetype-debugger;
         however, if --enable-freetype-debugger=DIR is specified, then DIR overrides
         the setting of FREETYPE_SOURCE])
FONTFORGE_ARG_WITH_GIFLIB
FONTFORGE_ARG_WITH_LIBJPEG
FONTFORGE_ARG_WITH_LIBPNG
FONTFORGE_ARG_WITH_LIBTIFF
FONTFORGE_ARG_WITH_CAIRO # do this test only 'after' zlib and png checks
FONTFORGE_ARG_WITH_LIBSPIRO
FONTFORGE_ARG_WITH_LIBUNINAMESLIST
#FONTFORGE_ARG_WITH_LIBUNICODENAMES
FONTFORGE_ARG_WITH_ZEROMQ
FONTFORGE_ARG_WITH_LIBREADLINE

FONTFORGE_ARG_ENABLE_WOFF2

if test x"${i_do_have_gui}" = xyes; then
   if test x"${i_do_have_libzmq}" = xyes; then
      AC_DEFINE(BUILD_COLLAB,[1],[Build the Collab Code])
   fi
fi

if test x"${i_do_have_cairo}" = xyes; then
   PKG_CHECK_MODULES([PANGOCAIRO],[pangocairo])
fi

FONTFORGE_CONFIG_FREETYPE_DEBUGGER

#--------------------------------------------------------------------------
#
# Checks for header files.

AC_CHECK_HEADERS([arpa/inet.h fcntl.h ieeefp.h])
AC_CHECK_HEADERS([langinfo.h libintl.h])
AC_CHECK_HEADERS([memory.h netdb.h netinet/in.h])
AC_CHECK_HEADERS([strings.h sys/ioctl.h sys/socket.h sys/time.h unistd.h])
if test x"${i_do_want_iconv}" = xyes; then
   AC_CHECK_HEADERS([iconv.h])
fi
AC_CHECK_HEADERS([execinfo.h])
gl_INIT

#--------------------------------------------------------------------------
#
# Checks for typedefs, structures, and compiler characteristics.

AC_C_BIGENDIAN
AC_TYPE_PID_T

# Error out under certain conditions, please.
gl_WARN_ADD([-Werror=implicit-function-declaration])
gl_WARN_ADD([-Werror=int-conversion])

#--------------------------------------------------------------------------
# Check to see if "this" compiler can use these flags, errors, or warnings.
# Enable the flags that can be used and skip flags that are unknown.
AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--enable-gcc-warnings],
                  [turn on lots of GCC warnings (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [gl_gcc_warnings=no]
)

if test "$gl_gcc_warnings" = yes; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])

  # Set up the list of undesired warnings.
  nw=
  nw="$nw -Wundef"            # All compiler preprocessors support #if UNDEF
  nw="$nw -Wtraditional"      # All compilers nowadays support ANSI C
  nw="$nw -Wstrict-overflow"  # Use a lower level (see below).
  nw="$nw -Wconversion"       # These warnings usually don’t point to mistakes.
  nw="$nw -Wsign-conversion"  # Likewise.
  nw="$nw -Wtraditional-conversion" # Don't care.
  nw="$nw -Wpadded"           # Don't care.
  nw="$nw -Wc++-compat"       # Don't care.
  nw="$nw -Woverlength-strings" # Don't care.
  nw="$nw -Wmissing-format-attribute" # Don't care.
  nw="$nw -Wshadow"           # Don't care.
  nw="$nw -Wunreachable-code" # Seems buggy.
  nw="$nw -Wsystem-headers"   # glibc headers fail as of glibc 2.17, gcc 4.8
  nw="$nw -Wsuggest-attribute=format" # glibc headers fail as of glibc 2.17, gcc 4.8
  nw="$nw -Wstrict-aliasing"  # -Sep2012, There's a bug in <py3 with Py_True/False that will propagate with GCC's "strict-aliasing" rules.
  nw="$nw -Wsuggest-attribute=pure" # FIXME: Maybe later?
  nw="$nw -Wbad-function-cast" # We do this on purpose (e.g. double to int)
  nw="$nw -Wall"
  nw="$nw -Wunused-macros"    # those macros might come in handy later

  gl_MANYWARN_ALL_GCC([warnings])

  # Enable all GCC warnings not in this list.
  gl_MANYWARN_COMPLEMENT([warnings], [$warnings], [$nw])
  for w in $warnings; do
    gl_WARN_ADD([$w])
  done

  # Add an extra warning
  gl_WARN_ADD([-Wformat])
  gl_WARN_ADD([-Wstrict-overflow=1])

  # Add these explicitly: adding -Wfoo to $nw doesn't work in this case
  gl_WARN_ADD([-Wno-unused-result]) # FIXME: We should in fact use this, but currently it makes too much noise
  gl_WARN_ADD([-Wno-comment]) # We use /* in comments with good reason

  # Add some more safety measures
  gl_WARN_ADD([-D_FORTIFY_SOURCE=2])

  # When compiling with GCC, prefer -isystem to -I when including system
  # include files, to avoid generating useless diagnostics for the files.
  ISYSTEM='-isystem '
else
  ISYSTEM='-I'
fi
AC_SUBST([ISYSTEM])

AC_LANG_PUSH([C]) dnl NOTE: configure.ac - try not to push/pop too often
# Create LDFLAGS for libraries. Currently used to pass '-no-undefined' to libtool.
my_lib_ldflags="${CODE_COVERAGE_LDFLAGS}"
dnl add --no-undefined if supported. Based on information seen here:
dnl https://mail.gnome.org/archives/commits-list/2011-October/msg10575.html
dnl http://femass.com.br/Net-SNMP/configure.d/config_os_progs
case $host in
   dnl FreeBSD (et al.) does not complete linking for shared objects when
   dnl pthreads are requested, as different implementations are present; to
   dnl avoid problems, use -Wl,-z,defs only for those platform not behaving
   dnl this way.
   *-freebsd* | *-openbsd*)
      ;;
   *-apple-darwin*)
      ;;
   *mingw*)
      dnl Explicitly export all symbols on Windows, since FontForge lacks an export table
	  dnl See issue #1238 on GitHub for more information.
	  my_lib_ldflags="${my_lib_ldflags} -no-undefined -Wl,--export-all-symbols"
      ;;
   *cygwin*)
	  my_lib_ldflags="${my_lib_ldflags} -Wl,-no-undefined -Wl,--export-all-symbols"
      ;;
   *) saved_LDFLAGS=$LDFLAGS
      AC_MSG_CHECKING([whether the linker supports --no-undefined])
      LDFLAGS="$saved_LDFLAGS -Wl,--no-undefined"
      AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
         [AC_MSG_RESULT([yes]);
          AX_CHECK_COMPILE_FLAG([-Wl,--no-undefined],[WARNING_CFLAGS+=" -Wl,--no-undefined"])],
         [AC_MSG_RESULT([no])])
      LDFLAGS="$saved_LDFLAGS"
	  my_lib_ldflags="${my_lib_ldflags} -no-undefined"
      ;;
esac
AC_SUBST([MY_LIB_LDFLAGS], [${my_lib_ldflags}])

# Native Windows Python modules have a '.pyd' extension and not '.dll'
pyhook_file_extension=""
case $host in
    *mingw*)
        pyhook_file_extension="${pyhook_file_extension} -shrext \".pyd\""
    ;;
esac
AC_SUBST([PYHOOK_FILE_EXTENSION], [${pyhook_file_extension}])

AC_LANG_POP

#--------------------------------------------------------------------------
# Bypass older autoscan recommendations (if anyone decides to run autoscan)
# These packages promises to not use malloc(0) or realloc(n, 0) so we don't
# want autoscan to suggest using AC_FUNC_MALLOC or AC_FUNC_REALLOC, by
# redefining the problem
m4_define([AN_FUNCTION], [m4_if([$1], [malloc], [],
  [AN_OUTPUT([function], $@)])])
m4_define([AN_FUNCTION], [m4_if([$1], [realloc], [],
  [AN_OUTPUT([function], $@)])])

#--------------------------------------------------------------------------
# Checks for library functions.

AC_FUNC_FORK
AC_FUNC_MKTIME
AC_FUNC_MMAP
if test x"${ac_cv_func_mmap_fixed_mapped}" != xyes; then
   AC_DEFINE([_NO_MMAP],1,[Define if not using mmap.])
fi
AC_CHECK_FUNCS([alarm dup2 endpwent])
AC_CHECK_FUNCS([getcwd gethostbyname gettimeofday])
AC_CHECK_FUNCS([mkdir munmap])
AC_CHECK_FUNCS([nl_langinfo putenv rmdir])
AC_CHECK_FUNCS([select setenv socket])
AC_CHECK_FUNCS([tzset uname])
AC_CHECK_FUNC([tzset],[:],[AC_DEFINE([_NO_TZSET],1,[Define if not using tzset.])])

#--------------------------------------------------------------------------
# Put ifndef wrapper on fontforge-config.h so we don't call it repeatedly.
AH_TOP([#ifndef _FF_CONFIG_H

#define _FF_CONFIG_H 1])
AH_BOTTOM([#endif])
#--------------------------------------------------------------------------

AM_CONDITIONAL([GRAPHICAL_USER_INTERFACE],[test x"${i_do_have_gui}" = xyes])
AM_CONDITIONAL([MACINTOSH],[test x"${gww_ismac}" = xyes])

# The following conditionals are always false, so the directories
# never get built by default. Nevertheless, they will be included by
# "make dist".
dnl AM_CONDITIONAL([FONTTOOLS],[test x != x])

#--------------------------------------------------------------------------
# Enable FontForge-Extras. Default is/was no since these files were default
# off (or not in use since 2012 or earlier). User can enable to build these
# files, and install extras found in directory contrib.
fontforge_extras=no
AC_ARG_ENABLE([fontforge-extras],
        [AS_HELP_STRING([--enable-fontforge-extras],
                [Build and Install fontforge-extras (from contrib directory).])],
        [fontforge_extras=yes])
# If user chose "./configure --disable-programs", then turn this off too.
if test x"${i_do_have_programs}" != xyes; then
   fontforge_extras=no
fi
AM_CONDITIONAL([FONTTOOLS],[test x"${fontforge_extras}" = xyes])

#--------------------------------------------------------------------------
# Create and Set FontForge Compiler Flags

my_cflags="${my_cflags} ${WARNING_CFLAGS}"
if test x"${i_do_have_freetype_debugger}" != xno; then
   my_cflags="${my_cflags} -I${FREETYPE_SOURCE}/src/truetype"
   my_cflags="${my_cflags} -I${FREETYPE_SOURCE}/include"
   my_cflags="${my_cflags} -I${FREETYPE_SOURCE}/include/freetype"
fi
my_cflags="${my_cflags} ${ZLIB_CFLAGS}"
my_cflags="${my_cflags} ${GLIB_CFLAGS}"
my_cflags="${my_cflags} ${CAIRO_CFLAGS}"
my_cflags="${my_cflags} ${PANGO_CFLAGS}"
my_cflags="${my_cflags} ${GDK_CFLAGS}"
my_cflags="${my_cflags} ${WOFF2_CFLAGS}"
my_cflags="${my_cflags} ${PANGOCAIRO_CFLAGS}"
my_cflags="${my_cflags} ${FREETYPE_CFLAGS}"
my_cflags="${my_cflags} ${GIFLIB_CFLAGS}"
my_cflags="${my_cflags} ${LIBJPEG_CFLAGS}"
my_cflags="${my_cflags} ${LIBPNG_CFLAGS}"
my_cflags="${my_cflags} ${LIBTIFF_CFLAGS}"
my_cflags="${my_cflags} ${LIBSPIRO_CFLAGS}"
my_cflags="${my_cflags} ${LIBUNINAMESLIST_CFLAGS}"
my_cflags="${my_cflags} ${LIBUNICODENAMES_CFLAGS}"
my_cflags="${my_cflags} ${LIBXML_CFLAGS}"
my_cflags="${my_cflags} ${PYTHON_CFLAGS}"
my_cflags="${my_cflags} ${PTHREAD_CFLAGS}"
my_cflags="${my_cflags} ${LIBREADLINE_CFLAGS}"
my_cflags="${my_cflags} ${CODE_COVERAGE_CFLAGS}"
test x"${SDK}" = x || my_cflags="${my_cflags} -I${SDK}"
test x"${LTDLINCL}" = x || my_cflags="${my_cflags} -I${LTDLINCL}"
AC_SUBST([MY_CFLAGS],[${my_cflags}])

#--------------------------------------------------------------------------
# Create and Set FontForge Library Flags

test x"${i_do_have_giflib}" = xyes && my_libs="${my_libs} ${GIFLIB_LIBS}"
test x"${i_do_have_libjpeg}" = xyes && my_libs="${my_libs} ${LIBJPEG_LIBS}"
test x"${i_do_have_libpng}" = xyes && my_libs="${my_libs} ${LIBPNG_LIBS}"
test x"${i_do_have_libtiff}" = xyes && my_libs="${my_libs} ${LIBTIFF_LIBS}"
test x"${i_do_have_libuninameslist}" = xyes && my_libs="${my_libs} ${LIBUNINAMESLIST_LIBS}"
test x"${i_do_have_libunicodenames}" = xyes && my_libs="${my_libs} ${LIBUNICODENAMES_LIBS}"
test x"${i_do_have_cairo}" = xyes && my_libs="${my_libs} ${CAIRO_LIBS}"
test x"${i_do_have_cairo}" = xyes && my_libs="${my_libs} ${PANGOCAIRO_LIBS}"
test x"${i_do_have_gui}" = xyes && my_libs="${my_libs} ${PANGO_LIBS}"
test x"${fontforge_can_use_woff2}" = xyes && my_libs="${my_libs} ${WOFF2_LIBS}"

if test x"${fontforge_can_use_gdk}" = xyes; then
    my_libs="${my_libs} ${GDK_LIBS}"
else
    test x"${i_do_have_x}" = xyes && my_libs="${my_libs} ${X_PRE_LIBS} ${X_LIBS} ${X_EXTRA_LIBS}"
fi

test x"${i_do_have_libreadline}" = xyes && my_libs="${my_libs} ${LIBREADLINE_LIBS}"
test x"${i_do_have_libspiro}" = xyes && my_libs="${my_libs} ${LIBSPIRO_LIBS}"
my_libs="${my_libs} ${LIBSPIRO_LIBS}"
my_libs="${my_libs} ${FREETYPE_LIBS}"
my_libs="${my_libs} ${PTHREAD_LIBS}"
my_libs="${my_libs} ${ZLIB_LIBS}"
my_libs="${my_libs} ${GLIB_LIBS}"
my_libs="${my_libs} ${LIBXML_LIBS}"
AC_SUBST([MY_LIBS],[${my_libs}])

#--------------------------------------------------------------------------

if test x"${i_do_have_python_scripting}" = xyes; then
   AC_MSG_CHECKING([for IPython])
   if $( $PYTHON -c 'import IPython' 2>&5 ); then
        have_ipython=yes;
        AC_DEFINE([HAVE_IPYTHON],[1],[have ipython])
        AC_MSG_RESULT([yes])
   else
        have_ipython=no
        AC_MSG_RESULT([no])
   fi
fi

AM_CONDITIONAL([HAVE_IPYTHON],[test x"${have_ipython}" = xyes])

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
#--------------------------------------------------------------------------

AC_ARG_ENABLE([python-even],
        [AS_HELP_STRING([--enable-python-even],
                [enable a menu entry for Even])],
        [have_python_even=yes])
AM_CONDITIONAL([HAVE_PYTHON_EVEN],[test x"${have_python_even}" = xyes && test x"${i_do_have_python_scripting}" = xyes])

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
#--------------------------------------------------------------------------

AC_ARG_ENABLE([breakpad],
        [AS_HELP_STRING([--enable-breakpad],
                [Use Breakpad for crash reporting (currently osx only)])],
        [have_breakpad=yes])
AM_CONDITIONAL([HAVE_BREAKPAD],[test x"${have_breakpad}" = xyes])
if test x$have_breakpad = xyes ; then
   echo "Taking advantage of Breakpad for debug reporting. You must roll a bundle for this to work..."
   BREAKPAD_LIBADD=' -L$(top_builddir)/osx/FontForge.app/Contents/Frameworks/Breakpad.framework -lBreakpad '
   AC_DEFINE(USE_BREAKPAD,[1],[Build support and use Breakpad])
fi
AC_SUBST(BREAKPAD_LIBADD)

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
# We do not use GTK2 unless explicitly told it is OK to try to do that
#
AC_ARG_ENABLE([gtk2-use],
        [AS_HELP_STRING([--enable-gtk2-use],
                [Try to detect and use gtk2])],
        [have_gtk2_use=yes])
if test x$have_gtk2_use = xyes ; then
    PKG_CHECK_MODULES([GTK2],[gtk+-2.0],
	[
		fontforge_can_use_gtk=yes
		AC_DEFINE(FONTFORGE_CAN_USE_GTK,[],[FontForge is able to use some GTK2 functions if it wants])
		echo "building functionality that relies on gtk2..."
	],
	[
		fontforge_can_use_gtk=no
		echo "NOT building functionality that relies on gtk2..."
	])
fi
AM_CONDITIONAL([FONTFORGE_CAN_USE_GTK],[test x"${fontforge_can_use_gtk}" = xyes])

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
# Output commands - Create FontForge Portable Makefiles

AC_CONFIG_FILES([
Makefile
inc/Makefile
lib/Makefile
libltdl/Makefile
Unicode/Makefile
gutils/Makefile
collab/Makefile
share/Makefile
gdraw/Makefile
fontforge/Makefile
fontforgeexe/Makefile
fontforgeexe/pixmaps/Makefile
fontforgeexe/pixmaps/2012/Makefile
fontforgeexe/pixmaps/tango/Makefile
fontforgeexe/collab/Makefile
po/Makefile
mackeys/Makefile
doc/Makefile
doc/html/Makefile
pycontrib/Makefile
pycontrib/simple/Makefile
pycontrib/collab/Makefile
pycontrib/graphicore/Makefile
pycontrib/gdraw/Makefile
nodejs/Makefile
nodejs/collabwebview/Makefile
nodejs/collabwebview/css/Makefile
nodejs/collabwebview/js/Makefile
nodejs/collabwebview/js/contentEditable/Makefile
pyhook/Makefile
plugins/Makefile
tests/Makefile
desktop/Makefile
contrib/Makefile
contrib/admintools/Makefile
contrib/cidmap/Makefile
contrib/cidmap/src/Makefile
contrib/fonttools/Makefile
osx/Makefile
osx/FontForge.app/Makefile
osx/FontForge.app/Contents/Makefile
osx/FontForge.app/Contents/Resources/Makefile
osx/FontForge.app/Contents/Resources/English.lproj/Makefile
osx/FontForge.app/Contents/Resources/opt/Makefile
osx/FontForge.app/Contents/Resources/opt/local/Makefile
osx/FontForge.app/Contents/Resources/opt/local/etc/Makefile
osx/FontForge.app/Contents/Resources/opt/local/etc/fonts/Makefile
osx/FontForge.app/Contents/Resources/breakpad/Makefile
osx/FontForge.app/Contents/Resources/breakpad/webinterface/Makefile
osx/FontForge.app/Contents/Resources/breakpad/webinterface/support/Makefile
osx/FontForge.app/Contents/MacOS/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/MacOS/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Inputbox.nib/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/MainMenu.nib/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Msgbox.nib/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/PopUpButton.nib/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Progressbar.nib/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/SecureInputbox.nib/Makefile
osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources/Textbox.nib/Makefile
osx/FontForge.app/Contents/Frameworks/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Headers/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Resources/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Resources/crash_report_sender.app/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Resources/crash_report_sender.app/Contents/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Resources/crash_report_sender.app/Contents/MacOS/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Resources/crash_report_sender.app/Contents/Resources/Makefile
osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Versions/A/Resources/crash_report_sender.app/Contents/Resources/English.lproj/Makefile
breakpad/Makefile
breakpad/server/Makefile

fontforge.pc
osx/FontForge.app/Contents/Info.plist
])

#--------------------------------------------------------------------------

FONTFORGE_CREATE_PKGCONFIG_FILES

#--------------------------------------------------------------------------
# Create FontForge Operating System Specific files
AC_CONFIG_FILES([fontforgeexe/darwinsetup],[chmod +x fontforgeexe/darwinsetup])

#--------------------------------------------------------------------------
# Create FontForge and Python test driver
AC_CONFIG_FILES([tests/atlocal])

#--------------------------------------------------------------------------
# Collect "GIT" compile-time information to help when debugging FontForge.

FONTFORGE_GIT_VERSION=""
if test -d ${srcdir}/.git; then
  FONTFORGE_GIT_VERSION=`cd ${srcdir} && git log --pretty=format:'%H' -n 1`
fi
FONTFORGE_DATE_NOW=${SOURCE_DATE_EPOCH:-$(date +'%s')}
FONTFORGE_DATE_OPTS="--date=@${FONTFORGE_DATE_NOW}"
if ! date --version 2>/dev/null ; then
  # Use POSIX date instead of GNU date on BSD systems
  FONTFORGE_DATE_OPTS="-j -f %s ${FONTFORGE_DATE_NOW}"
fi
FONTFORGE_MODTIME="$(date -u $FONTFORGE_DATE_OPTS +'%s')L"
FONTFORGE_MODTIME_STR="$(date -u $FONTFORGE_DATE_OPTS +'%H:%M %Z %e-%b-%Y')"
FONTFORGE_VERSIONDATE="$(date -u $FONTFORGE_DATE_OPTS +'%Y%m%d')"
AC_DEFINE_UNQUOTED(FONTFORGE_GIT_VERSION,"$FONTFORGE_GIT_VERSION","git hash that source are built from")
AC_DEFINE_UNQUOTED(FONTFORGE_MODTIME,"$FONTFORGE_MODTIME","Epoch time that configure was run at")
AC_DEFINE_UNQUOTED(FONTFORGE_MODTIME_STR,"$FONTFORGE_MODTIME_STR",
	"A user friendly date that that configure was run at: For example 01:48 GMT 6-Sep-2012")
AC_DEFINE_UNQUOTED(FONTFORGE_VERSIONDATE,"$FONTFORGE_VERSIONDATE","YYYYMMDD as number when configure was run")
AC_DEFINE_UNQUOTED(FONTFORGE_MODTIME_RAW,$FONTFORGE_MODTIME,"Epoch time that configure was run at (no quotes)")
AC_DEFINE_UNQUOTED(FONTFORGE_MODTIME_STR_RAW,$FONTFORGE_MODTIME_STR,
	"A user friendly date that that configure was run at: For example 01:48 GMT 6-Sep-2012 (no quotes)")
AC_DEFINE_UNQUOTED(FONTFORGE_VERSIONDATE_RAW,$FONTFORGE_VERSIONDATE,"YYYYMMDD as number when configure was run (no quotes)")
AC_SUBST(FONTFORGE_GIT_VERSION)
AC_SUBST(FONTFORGE_VERSIONDATE)
AC_SUBST(FONTFORGE_MODTIME_STR)

#--------------------------------------------------------------------------

AC_OUTPUT

AC_MSG_NOTICE([

Configuration:

  Source code location	${srcdir}
  Build code location	${builddir}
  Destination prefix	${prefix}
  Compiler		${CC}

Summary of optional features:

			UseIt?	HaveIt?
  real (floating pt)		${my_real_type}
  programs		yes	${i_do_have_programs}
  native scripting	yes	${i_do_have_native_scripting}
  python scripting	${enable_python_scripting}	${i_do_have_python_scripting}${i_want_python_ver}
  python extension	${enable_python_extension}	${i_do_have_python_extension}
  freetype debugger		${i_do_have_freetype_debugger}
  raw points mode		${i_do_have_debug_raw_points}
  tile path			${i_do_have_tile_path}
  gb12345 encoding		${i_do_have_gb12345}
  fontforge-extras	yes	${fontforge_extras}
  potrace or autotrace	${i_do_find_auto_po_trace}

Summary of optional dependencies:

Optional Library	UseIt?	HaveIt?	WebsiteURL
  cairo			${with_cairo}	${i_do_have_cairo}	${cairo_url}
  giflib		${with_giflib}	${i_do_have_giflib}	${giflib_url}
  libjpeg		${with_libjpeg}	${i_do_have_libjpeg}	${libjpeg_url}
  libpng		${with_libpng}	${i_do_have_libpng}	${libpng_url}
  libreadline		${with_libreadline}	${i_do_have_libreadline}	${libreadline_url}
  libspiro		${with_libspiro}	${i_do_have_libspiro}	${libspiro_url}
  libtiff		${with_libtiff}	${i_do_have_libtiff}	${libtiff_url}
  libuninameslist	${with_libuninameslist}	${i_do_have_libuninameslist}	${libuninameslist_url}
  python-dev		${enable_python_scripting}	${i_do_have_python_scripting}	${python_url}
  zeromq (libzmq)		${i_do_have_libzmq}	${libzmq_url}
  woff2			${fontforge_can_use_woff2}	${fontforge_has_woff2}	${woff2_url}
  X Window System		${i_do_have_x}	${libxorg_url}
  GDK backend		${fontforge_can_use_gdk}	${fontforge_gdk_version}	${gdk_url}
])

#--------------------------------------------------------------------------
