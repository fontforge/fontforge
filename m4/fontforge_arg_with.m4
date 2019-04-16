dnl -*- autoconf -*-

dnl If you leave out package-specific-hack, and no pkg-config module is found,
dnl then PKG_CHECK_MODULES will fail with an informative message.
dnl
dnl FONTFORGE_ARG_WITH_BASE(package, help-message, pkg-config-name, not-found-warning-message, _NO_XXXXX,
dnl                         [package-specific-hack])
dnl -----------------------------------------------------------------------------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_BASE],
[
AC_ARG_WITH([$1],[$2],
        [eval AS_TR_SH(i_do_have_$1)="${withval}"],
        [eval AS_TR_SH(i_do_have_$1)=no])
if test x"${AS_TR_SH(i_do_have_$1)}" = xyes; then
   # First try pkg-config, then try a possible package-specific hack.
   PKG_CHECK_MODULES(m4_toupper([$1]),[$3],[:],[$6])
fi
if test x"${AS_TR_SH(i_do_have_$1)}" != xyes; then
   $4
   AC_DEFINE([$5],1,[Define if not using $1.])
fi
AM_CONDITIONAL(m4_toupper([$1]), test x"${AS_TR_SH(i_do_have_$1)}" = xyes)
])


dnl Call this if you want to use pkg-config and don't have a package-specific hack.
dnl
dnl FONTFORGE_ARG_WITH(package, help-message, pkg-config-name, not-found-warning-message, _NO_XXXXX)
dnl ------------------------------------------------------------------------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH],
   [FONTFORGE_ARG_WITH_BASE([$1],[$2],[$3],[$4],[$5],[eval AS_TR_SH(i_do_have_$1)=no])])


dnl FONTFORGE_ARG_WITHOUT(library_name, LIBRARY_NAME, without-help-message)
dnl -----------------------------------------------------------------------
dnl Commonly repeated code used in the --without-lib_name routines to create
dnl CFLAGS and LIBS, plus prompt user if they want to build with or without.
dnl default is to set as 'check' unless the user specifies 'yes' or 'no'
AC_DEFUN([FONTFORGE_ARG_WITHOUT],[
AC_ARG_VAR([$2_CFLAGS],[C compiler flags for $2, overriding automatic detection])
AC_ARG_VAR([$2_LIBS],[Linker flags for $2, overriding automatic detection])

AC_ARG_WITH([$1],[AS_HELP_STRING([--without-$1],[$3])],
        [AS_TR_SH(i_do_have_$1)="${withval}"; AS_TR_SH(with_$1)="${withval}"],
        [AS_TR_SH(i_do_have_$1)=yes; AS_TR_SH(with_$1)=check])
])


dnl FONTFORGE_BUILD_YES_NO_HALT(library_name, LIBRARY_NAME, build_message)
dnl ----------------------------------------------------------------------
dnl Commonly repeated code used in the --without-lib_name routines to test
dnl whether to continue or create a forced-halt. The forced-halt seems to
dnl be necessary since some superscripts such as homebrew (see issue#1366)
dnl that call ./bootstrap or ./autogen.sh enable a quiet mode where users
dnl won't see why ./configure fails. This forced-halt also aids users by
dnl giving an immediate answer as to why ./configure fails (see lots of
dnl issues before this where users run into problems, but reason for fail
dnl isn't immediately clear) - answer is usually in adding a developer pkg
dnl since some distros normally distribute compiled binaries to save space.
dnl This also avoids using pkg-config since some versions don't check the
dnl additional directories such as /usr/local by default without users
dnl adding additional mods to make them check these other sub-directories.
dnl Function initially tested-out in function FONTFORGE_ARG_WITH_LIBSPIRO()
AC_DEFUN([FONTFORGE_BUILD_YES_NO_HALT],[
AC_MSG_CHECKING([$3])
if test x"${AS_TR_SH(with_$1)}" = xyes; then
   if test x"${AS_TR_SH(i_do_have_$1)}" != xno; then
      AC_MSG_RESULT([yes])
   else
      AC_MSG_FAILURE([ERROR: Please install the Developer version of $1],[1])
   fi
else
   if test x"${AS_TR_SH(i_do_have_$1)}" = xno || test x"${AS_TR_SH(with_$1)}" = xno; then
      AC_MSG_RESULT([no])
      AC_DEFINE([_NO_$2],1,[Define if not using $1])
      AS_TR_SH($2_CFLAGS)=""
      AS_TR_SH($2_LIBS)=""
   else
      AC_MSG_RESULT([yes])
   fi
fi
AC_SUBST([$2_CFLAGS])
AC_SUBST([$2_LIBS])
])


dnl FONTFORGE_ARG_WITH_LIBUNINAMESLIST
dnl ----------------------------------
dnl Add with libuninameslist support by default (only if the libuninameslist
dnl library exists AND uninameslist.h header file exist). If both are found,
dnl then check further to see if this version of libuninameslist also has
dnl uniNamesList_NamesListVersion(). If uniNamesList_NamesListVersion() also
dnl exists, then set _LIBUNINAMESLIST_FUN=3 since this is version >= 0.3.
dnl If uniNamesList_blockCount() also exists, then _LIBUNINAMESLIST_FUN>=4
dnl If user defines --without-libuninameslist, then don't use libuninameslist.
dnl If user defines --with-libuninameslist, then fail with error if there is
dnl no libuninameslist library OR no uninameslist.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNINAMESLIST],[
FONTFORGE_ARG_WITHOUT([libuninameslist],[LIBUNINAMESLIST],[build without Unicode Name or Annotation support])

if test x"${i_do_have_libuninameslist}" = xyes -a x"${LIBUNINAMESLIST_CFLAGS}" = x; then
AC_CHECK_HEADER([uninameslist.h],[],[i_do_have_libuninameslist=no])
fi
if test x"${i_do_have_libuninameslist}" = xyes -a x"${LIBUNINAMESLIST_LIBS}" = x; then
   have_libuninameslist=0
   FONTFORGE_SEARCH_LIBS([UnicodeNameAnnot],[uninameslist],
      [LIBUNINAMESLIST_LIBS="${LIBUNINAMESLIST_LIBS} ${found_lib}"
       AC_CHECK_FUNC([uniNamesList_NamesListVersion],[have_libuninameslist=3])
       AC_CHECK_FUNC([uniNamesList_blockCount],[have_libuninameslist=4])
       AC_CHECK_FUNC([uniNamesList_names2anU],[have_libuninameslist=5])
       AC_DEFINE_UNQUOTED([_LIBUNINAMESLIST_FUN],[$have_libuninameslist],[LibUninNamesList library >= 0.3])
      ],[i_do_have_libuninameslist=no])
fi

FONTFORGE_BUILD_YES_NO_HALT([libuninameslist],[LIBUNINAMESLIST],[Build with LibUniNamesList Unicode support?])

AC_DEFINE([_NO_LIBUNICODENAMES],[1],[Define if not using libunicodenames])
i_do_have_libunicodenames=no
])


dnl FONTFORGE_ARG_WITH_LIBUNICODENAMES
dnl ----------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNICODENAMES],
[
AC_ARG_VAR([LIBUNICODENAMES_CFLAGS],[C compiler flags for LIBUNICODENAMES, overriding the automatic detection])
AC_ARG_VAR([LIBUNICODENAMES_LIBS],[linker flags for LIBUNICODENAMES, overriding the automatic detection])
AC_ARG_WITH([libunicodenames],[AS_HELP_STRING([--without-libunicodenames],
	    [build without Unicode Name or Annotation support])],
	    [i_do_have_libunicodenames="${withval}"],[i_do_have_libunicodenames=yes])
if test x"${i_do_have_libunicodenames}" = xyes -a x"${LIBUNICODENAMES_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([uninm_names_db_open],[unicodenames],
		  [LIBUNICODENAMES_LIBS="${LIBUNICODENAMES_LIBS} ${found_lib}"],
		  [i_do_have_libunicodenames=no])
   if test x"${i_do_have_libunicodenames}" != xyes; then
      i_do_have_libunicodenames=yes
      unset ac_cv_search_uninm_names_db_open
      FONTFORGE_SEARCH_LIBS([uninm_names_db_open],[unicodenames],
		     [LIBUNICODENAMES_LIBS="${LIBUNICODENAMES_LIBS} ${found_lib} -lunicodenames"],
                     [i_do_have_libunicodenames=no],
		     [-lunicodenames])
   fi
fi
if test x"${i_do_have_libunicodenames}" = xyes -a x"${LIBUNICODENAMES_CFLAGS}" = x; then
   AC_CHECK_HEADER([libunicodenames.h],[AC_SUBST([LIBUNICODENAMES_CFLAGS],[""])],[i_do_have_libunicodenames=no])
fi
if test x"${i_do_have_libunicodenames}" = xyes; then
   if test x"${LIBUNICODENAMES_LIBS}" != x; then
      AC_SUBST([LIBUNICODENAMES_LIBS],["${LIBUNICODENAMES_LIBS}"])
   fi
else
   FONTFORGE_WARN_PKG_NOT_FOUND([LIBUNICODENAMES])
   AC_DEFINE([_NO_LIBUNICODENAMES],1,[Define if not using libunicodenames.])
fi
AC_DEFINE([_NO_LIBUNINAMESLIST],[1],[Define if not using libuninameslist library])
i_do_have_libuninameslist=nocheck
with_libuninameslist=no
])


dnl FONTFORGE_ARG_WITH_CAIRO
dnl ------------------------
dnl Add with cairo support by default (only if cairo and headers exist).
dnl Additionally, zlib, png and their header files should exist too.
dnl If user defines --without-cairo, then don't use cairo graphics.
dnl If user defines --with-cairo, then fail with error if there is no
dnl cairo or headers available.
dnl You will need to test for zlib and png before running this function.
dnl Latest version here indicated cairo >= 1.6
dnl function=`cairo_format_stride_for_width` will verify cairo >= 1.6
AC_DEFUN([FONTFORGE_ARG_WITH_CAIRO],[
FONTFORGE_ARG_WITHOUT([cairo],[LIBCAIRO],[build without Cairo graphics (use regular X graphics instead)])

if test x"${with_cairo}" != xno; then
dnl try the easy path first with PKG_CHECK_MODULES() else check using
dnl lib and header checks for distros/OSes with weak pkg-config setups.
   PKG_CHECK_MODULES([LIBCAIRO],[cairo >= 1.6],[i_do_have_cairo=yes],
         [if test x"${i_do_have_cairo}" != xno -a x"${LIBCAIRO_CFLAGS}" = x; then
             AC_CHECK_HEADER([cairo/cairo.h],[],[i_do_have_cairo=no])
          fi
          if test x"${i_do_have_cairo}" != xno -a x"${LIBCAIRO_LIBS}" = x; then
             FONTFORGE_SEARCH_LIBS([cairo_format_stride_for_width],[cairo],
                   [LIBCAIRO_LIBS="${LIBCAIRO_LIBS} ${found_lib}"],
                   [i_do_have_cairo=no])
          fi])
fi

AC_MSG_CHECKING([Build with Cairo support?])
if test x"${with_cairo}" != xno; then
   if test x"${i_do_have_libpng}" != xyes || test x"${with_libpng}" = xno; then
      AC_MSG_FAILURE([ERROR: Please install the Developer version of libpng],[1])
   fi
fi
if test x"${with_cairo}" = xyes; then
   if test x"${i_do_have_cairo}" != xno; then
      AC_MSG_RESULT([yes])
   else
      AC_MSG_FAILURE([ERROR: Please install the Developer version of Cairo],[1])
   fi
else
   if test x"${i_do_have_cairo}" = xno || test x"${with_cairo}" = xno; then
      AC_MSG_RESULT([no])
      AC_DEFINE([_NO_LIBCAIRO],1,[Define if not using cairo])
      AS_TR_SH(LIBCAIRO_CFLAGS)=""
      AS_TR_SH(LIBCAIRO_LIBS)=""
   else
      AC_MSG_RESULT([yes])
   fi
fi
AC_SUBST([LIBCAIRO_CFLAGS])
AC_SUBST([LIBCAIRO_LIBS])
])


dnl FONTFORGE_ARG_WITH_LIBPNG
dnl -------------------------
dnl Add with libpng support by default (only if the libpng library exists
dnl AND png.h header file exist).
dnl If user defines --without-libpng, then don't use libpng.
dnl If user defines --with-libpng, then fail with error if there is no
dnl libpng library OR no png.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBPNG],[
FONTFORGE_ARG_WITHOUT([libpng],[LIBPNG],[build without PNG support])

if test x"${i_do_have_libpng}" = xyes -a x"${LIBPNG_CFLAGS}" = x; then
   AC_CHECK_HEADER([png.h],[],[i_do_have_libpng=no])
fi
if test x"${i_do_have_libpng}" = xyes -a x"${LIBPNG_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([png_init_io],[png],
         [LIBPNG_LIBS="${LIBPNG_LIBS} ${found_lib}"],
         [i_do_have_libpng=no])
fi

FONTFORGE_BUILD_YES_NO_HALT([libpng],[LIBPNG],[Build with PNG support?])
])


dnl FONTFORGE_ARG_WITH_LIBTIFF
dnl --------------------------
dnl Add with libtiff support by default (only if the libtiff library exists
dnl AND tiffio.h header file exist). First, verify that TIFFRewriteField is
dnl NOT accessible, otherwise this is an old tiff library (less than v4.0).
dnl libtiff 4 and higher have bigtiff support.
dnl If user defines --without-libtiff, then don't use libtiff.
dnl If user defines --with-libtiff, then fail with error if there is no
dnl libtiff library OR libtiff < ver4.0, OR no tiffio.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBTIFF],[
FONTFORGE_ARG_WITHOUT([libtiff],[LIBTIFF],[build without TIFF support])

if test x"${i_do_have_libtiff}" = xyes -a x"${LIBTIFF_CFLAGS}" = x; then
   AC_CHECK_HEADER([tiffio.h],[],[i_do_have_libtiff=no])
fi
if test x"${i_do_have_libtiff}" = xyes -a x"${LIBTIFF_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([TIFFRewriteField],[tiff],
         [i_do_have_libtiff=no],[])
   if test x"${i_do_have_libtiff}" = xyes; then
      FONTFORGE_SEARCH_LIBS([TIFFClose],[tiff],
            [LIBTIFF_LIBS="${LIBTIFF_LIBS} ${found_lib}"],
            [i_do_have_libtiff=no])
   fi
fi

FONTFORGE_BUILD_YES_NO_HALT([libtiff],[LIBTIFF],[Build with TIFF support?])
])


dnl FONTFORGE_ARG_WITH_LIBREADLINE
dnl ------------------------------
dnl Add with libreadline support by default, only if libreadline library exists
dnl AND readline/readline.h header file exist.
dnl If user defines --without-libreadline, then do not include libreadline.
dnl If user defines --with-libreadline, then fail with error if there is no
dnl libreadline library OR no readline/readline.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBREADLINE],[
FONTFORGE_ARG_WITHOUT([libreadline],[LIBREADLINE],[build without READLINE support])

if test x"${i_do_have_libreadline}" = xyes -a x"${LIBREADLINE_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([rl_readline_version],[readline],
		  [LIBREADLINE_LIBS="${LIBREADLINE_LIBS} ${found_lib}"],
                  [i_do_have_libreadline=no])
   if test x"${i_do_have_libreadline}" != xyes; then
      i_do_have_libreadline=yes
      unset ac_cv_search_rl_readline_version
      FONTFORGE_SEARCH_LIBS([rl_readline_version],[readline],
		     [LIBREADLINE_LIBS="${LIBREADLINE_LIBS} ${found_lib} -ltermcap"],
                     [i_do_have_libreadline=no],
		     [-ltermcap])
   fi
fi
if test x"${i_do_have_libreadline}" = xyes -a x"${LIBREADLINE_CFLAGS}" = x; then
   AC_CHECK_HEADER([readline/readline.h],[LIBREADLINE_CFLAGS=""],[i_do_have_libreadline=no])
fi

FONTFORGE_BUILD_YES_NO_HALT([libreadline],[LIBREADLINE],[Build with LibReadLine support?])
])


dnl FONTFORGE_ARG_WITH_LIBSPIRO
dnl ---------------------------
dnl Add with libspiro support by default (only if libspiro library exists AND
dnl spiroentrypoints.h header file exist). If both found, then check further
dnl to see if this version of libspiro also has TaggedSpiroCPsToBezier0().
dnl If LibSpiroGetVersion() also exists, then also set _LIBSPIRO_FUN=2, else
dnl if TaggedSpiroCPsToBezier0() also exists, then also set _LIBSPIRO_FUN=1
dnl If user defines --without-libspiro, then do not include libspiro.
dnl If user defines --with-libspiro, then fail with error if there is no
dnl libspiro library OR no spiroentrypoints.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBSPIRO],[
FONTFORGE_ARG_WITHOUT([libspiro],[LIBSPIRO],[build without support for Spiro contours])

if test x"${i_do_have_libspiro}" = xyes -a x"${LIBSPIRO_CFLAGS}" = x; then
   AC_CHECK_HEADER([spiroentrypoints.h],[],[i_do_have_libspiro=no])
fi
if test x"${i_do_have_libspiro}" = xyes -a x"${LIBSPIRO_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([TaggedSpiroCPsToBezier],[spiro],
         [LIBSPIRO_LIBS="${LIBSPIRO_LIBS} ${found_lib}"
          AC_CHECK_FUNC([LibSpiroVersion],
                [AC_DEFINE([_LIBSPIRO_FUN],[2],[LibSpiro >= 0.6, includes LibSpiroVersion()])],
          AC_CHECK_FUNC([TaggedSpiroCPsToBezier0],
                [AC_DEFINE([_LIBSPIRO_FUN],[1],[LibSpiro >= 0.2, includes TaggedSpiroCPsToBezier0()])]))
         ],[i_do_have_libspiro=no])
fi

FONTFORGE_BUILD_YES_NO_HALT([libspiro],[LIBSPIRO],[Build with LibSpiro Curve Contour support?])
])


dnl FONTFORGE_ARG_WITH_GIFLIB
dnl -------------------------
dnl Add with giflib or libungif support by default (only if library AND header
dnl file exists). If both found, then check for enhanced DGifOpenFileName
dnl If user defines --without-giflib, then do not include giflib or libungif.
dnl If user defines --with-giflib, then fail with error if there is no
dnl giflib or libungif library OR no gif_lib.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_GIFLIB],[
FONTFORGE_ARG_WITHOUT([giflib],[GIFLIB],[build without GIF or LIBUNGIF support])

if test x"${i_do_have_giflib}" = xyes -a x"${GIFLIB_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([DGifOpenFileName],[gif ungif],
         [GIFLIB_LIBS="${GIFLIB_LIBS} ${found_lib}"],[i_do_have_giflib=no])
   dnl version 5+ breaks basic interface, so must use enhanced "DGifOpenFileName"
   FONTFORGE_SEARCH_LIBS([EGifGetGifVersion],[gif ungif],
         [AC_SUBST([_GIFLIB_5PLUS],["${found_lib}"])],[])
fi
if test x"${i_do_have_giflib}" = xyes -a x"${GIFLIB_CFLAGS}" = x; then
   AC_CHECK_HEADER(gif_lib.h,[],[i_do_have_giflib=no])
   if test x"${i_do_have_giflib}" = xyes; then
      AC_CACHE_CHECK([for ExtensionBlock.Function in gif_lib.h],
        ac_cv_extensionblock_in_giflib,
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <gif_lib.h>],[ExtensionBlock foo; foo.Function=3;])],
          [ac_cv_extensionblock_in_giflib=yes],[ac_cv_extensionblock_in_giflib=no])])
      if test x"${ac_cv_extensionblock_in_giflib}" != xyes; then
         AC_MSG_WARN([FontForge found giflib or libungif but cannot use this version.])
         i_do_have_giflib=no
      fi
   fi
fi

FONTFORGE_BUILD_YES_NO_HALT([giflib],[GIFLIB],[Build with GIFLIB or LIBUNGIF support?])
if test x"${i_do_have_giflib}" != xyes; then
   AC_DEFINE([_NO_LIBUNGIF],1,[Define if not using giflib or libungif.)])
fi
])


dnl FONTFORGE_ARG_WITH_LIBJPEG
dnl --------------------------
dnl Add libjpeg support by default (only if library AND header exist).
dnl If user defines --without-libjpeg, then do not include libjpeg.
dnl If user defines --with-libjpeg, then fail and stop with error if
dnl there is no libjpeg library OR no jpeglib.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBJPEG],[
FONTFORGE_ARG_WITHOUT([libjpeg],[LIBJPEG],[build without JPEG support])

if test x"${i_do_have_libjpeg}" = xyes -a x"${LIBJPEG_CFLAGS}" = x; then
   AC_CHECK_HEADER([jpeglib.h],[],[i_do_have_libjpeg=no])
fi
if test x"${i_do_have_libjpeg}" = xyes -a x"${LIBJPEG_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([jpeg_CreateDecompress],[jpeg],
         [LIBJPEG_LIBS="${LIBJPEG_LIBS} ${found_lib}"],
         [i_do_have_libjpeg=no])
fi

FONTFORGE_BUILD_YES_NO_HALT([libjpeg],[LIBJPEG],[Build with JPEG support?])
])

dnl FONTFORGE_ARG_WITH_ICONV
dnl --------------------------
dnl Add iconv support by default
AC_DEFUN([FONTFORGE_ARG_WITH_ICONV],[
FONTFORGE_ARG_WITHOUT([iconv],[ICONV],[build without iconv support])

if test x"${i_do_have_iconv}" = xyes -a x"${ICONV_CFLAGS}" = x; then
   AC_CHECK_HEADER([iconv.h],[],[i_do_have_iconv=no])
fi
if test x"${i_do_have_iconv}" = xyes -a x"${ICONV_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([libiconv_open],[iconv],
         [ICONV_LIBS="${ICONV_LIBS} ${found_lib}"],
         [
            FONTFORGE_SEARCH_LIBS([iconv_open], [iconv],
                [ICONV_LIBS="${ICONV_LIBS} ${found_lib}"],
                [i_do_have_iconv=no])
         ])
fi

if test x"${i_do_have_iconv}" = xyes; then
    AC_DEFINE(HAVE_ICONV_H,[1],[Build with iconv support])
fi

])


dnl FONTFORGE_WARN_PKG_NOT_FOUND
dnl ----------------------------
AC_DEFUN([FONTFORGE_WARN_PKG_NOT_FOUND],
   [AC_MSG_WARN([$1 was not found. We will continue building without it.])])


dnl FONTFORGE_WARN_PKG_FALLBACK
dnl ---------------------------
AC_DEFUN([FONTFORGE_WARN_PKG_FALLBACK],
   [AC_MSG_WARN([No pkg-config file was found for $1, but the library is present and we will try to use it.])])
