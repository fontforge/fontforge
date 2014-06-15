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
        [eval AS_TR_SH(i_do_have_$1)=yes])
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

AC_CHECK_HEADER([uninameslist.h],[i_do_have_libuninameslist=yes],[i_do_have_libuninameslist=no])
if test x"${i_do_have_libuninameslist}" = xyes; then
   have_libuninameslist=0
   FONTFORGE_SEARCH_LIBS([UnicodeNameAnnot],[uninameslist],
      [LIBUNINAMESLIST_LIBS="${LIBUNINAMESLIST_LIBS} ${found_lib}"
       AC_CHECK_FUNC([uniNamesList_NamesListVersion],[have_libuninameslist=3])
       AC_CHECK_FUNC([uniNamesList_blockCount],[have_libuninameslist=4])
       AC_DEFINE_UNQUOTED([_LIBUNINAMESLIST_FUN],[$have_libuninameslist],[LibUninNamesList library >= 0.3])
      ],[i_do_have_libuninameslist=no])
fi

AC_MSG_CHECKING([Build with LibUniNamesList Unicode support?])
if test x"${with_libuninameslist}" = xyes; then
   if test x"${i_do_have_libuninameslist}" != xno; then
      AC_MSG_RESULT([yes])
   else
      AC_MSG_FAILURE([ERROR: Please install the Developer version of libuninameslist],[1])
   fi
else
   if test x"${i_do_have_libuninameslist}" = xno || test x"${with_libuninameslist}" = xno; then
      AC_MSG_RESULT([no])
      AC_DEFINE([_NO_LIBUNINAMESLIST],[1],[Define if not using libuninameslist library])
      LIBUNINAMESLIST_CFLAGS=""
      LIBUNINAMESLIST_LIBS=""
   else
      AC_MSG_RESULT([yes])
   fi
fi
AC_SUBST([LIBUNINAMESLIST_CFLAGS])
AC_SUBST([LIBUNINAMESLIST_LIBS])

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
AC_DEFUN([FONTFORGE_ARG_WITH_CAIRO],
[
   FONTFORGE_ARG_WITH([cairo],
      [AS_HELP_STRING([--without-cairo],
                      [build without Cairo graphics (use regular X graphics instead)])],
      [cairo >= 1.6],
      [FONTFORGE_WARN_PKG_NOT_FOUND([CAIRO])],
      [_NO_LIBCAIRO])
])


dnl FONTFORGE_ARG_WITH_LIBPNG
dnl -------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBPNG],
[
FONTFORGE_ARG_WITH([libpng],
        [AS_HELP_STRING([--without-libpng],[build without PNG support])],
        [libpng],
        [FONTFORGE_WARN_PKG_NOT_FOUND([LIBPNG])],
        [_NO_LIBPNG])
])


dnl FONTFORGE_ARG_WITH_LIBTIFF
dnl --------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBTIFF],
[
FONTFORGE_ARG_WITH_BASE([libtiff],
        [AS_HELP_STRING([--without-libtiff],[build without TIFF support])],
        [libtiff-4],
        [FONTFORGE_WARN_PKG_NOT_FOUND([LIBTIFF])],
        [_NO_LIBTIFF],
        [FONTFORGE_ARG_WITH_LIBTIFF_fallback])
])
dnl
AC_DEFUN([FONTFORGE_ARG_WITH_LIBTIFF_fallback],
[
   FONTFORGE_SEARCH_LIBS([TIFFClose],[tiff],
      [i_do_have_libtiff=yes
       AC_SUBST([LIBTIFF_CFLAGS],[""])
       AC_SUBST([LIBTIFF_LIBS],["${found_lib}"])
       FONTFORGE_WARN_PKG_FALLBACK([LIBTIFF])],
      [i_do_have_libtiff=no])
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
   AC_CHECK_HEADER([readline/readline.h],[$LIBREADLINE_CFLAGS=""],[i_do_have_libreadline=no])
fi

AC_MSG_CHECKING([Build with LibReadLine support?])
if test x"${with_libreadline}" = xyes; then
   if test x"${i_do_have_libreadline}" != xno; then
      AC_MSG_RESULT([yes])
   else
      AC_MSG_FAILURE([ERROR: Please install the Developer version of libreadline],[1])
   fi
else
   if test x"${i_do_have_libreadline}" = xno || test x"${with_libreadline}" = xno; then
      AC_MSG_RESULT([no])
      AC_DEFINE([_NO_LIBREADLINE],1,[Define if not using libreadline.])
      LIBREADLINE_CFLAGS=""
      LIBREADLINE_LIBS=""
   else
      AC_MSG_RESULT([yes])
   fi
fi
AC_SUBST([LIBREADLINE_CFLAGS])
AC_SUBST([LIBREADLINE_LIBS])
])


dnl FONTFORGE_ARG_WITH_LIBSPIRO
dnl ---------------------------
dnl Add with libspiro support by default (only if libspiro library exists AND
dnl spiroentrypoints.h header file exist). If both found, then check further
dnl to see if this version of libspiro also has TaggedSpiroCPsToBezier0().
dnl If TaggedSpiroCPsToBezier0() also exists, then also set _LIBSPIRO_FUN=1
dnl If user defines --without-libspiro, then do not include libspiro.
dnl If user defines --with-libspiro, then fail with error if there is no
dnl libspiro library OR no spiroentrypoints.h header file.
AC_DEFUN([FONTFORGE_ARG_WITH_LIBSPIRO],[
FONTFORGE_ARG_WITHOUT([libspiro],[LIBSPIRO],[build without support for Spiro contours])

AC_CHECK_HEADER([spiroentrypoints.h],[i_do_have_libspiro=yes],[i_do_have_libspiro=no])
if test x"${i_do_have_libspiro}" = xyes; then
   FONTFORGE_SEARCH_LIBS([TaggedSpiroCPsToBezier],[spiro],
         [LIBSPIRO_LIBS="${LIBSPIRO_LIBS} ${found_lib}"
          AC_CHECK_FUNC([TaggedSpiroCPsToBezier0],
                [AC_DEFINE([_LIBSPIRO_FUN],[1],[LibSpiro >= 0.2, includes TaggedSpiroCPsToBezier0()])])],
         [i_do_have_libspiro=no])
fi

AC_MSG_CHECKING([Build with libspiro support?])
if test x"${with_libspiro}" = xyes; then
   if test x"${i_do_have_libspiro}" != xno; then
      AC_MSG_RESULT([yes])
   else
      AC_MSG_FAILURE([ERROR: Please install the Developer version of libspiro],[1])
   fi
else
   if test x"${i_do_have_libspiro}" = xno || test x"${with_libspiro}" = xno; then
      AC_MSG_RESULT([no])
      AC_DEFINE([_NO_LIBSPIRO],[1],[Define if not using libspiro library])
      LIBSPIRO_CFLAGS=""
      LIBSPIRO_LIBS=""
   else
      AC_MSG_RESULT([yes])
   fi
fi
AC_SUBST([LIBSPIRO_CFLAGS])
AC_SUBST([LIBSPIRO_LIBS])
])


dnl There is no pkg-config support for giflib, at least on Gentoo. (17 Jul 2012)
dnl
dnl FONTFORGE_ARG_WITH_GIFLIB
dnl -------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_GIFLIB],
[
AC_ARG_VAR([GIFLIB_CFLAGS],[C compiler flags for GIFLIB, overriding the automatic detection])
AC_ARG_VAR([GIFLIB_LIBS],[linker flags for GIFLIB, overriding the automatic detection])
AC_ARG_WITH([giflib],[AS_HELP_STRING([--without-giflib],[build without GIF support])],
            [i_do_have_giflib="${withval}"],[i_do_have_giflib=yes])
if test x"${i_do_have_giflib}" = xyes -a x"${GIFLIB_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([DGifOpenFileName],[gif ungif],
                  [AC_SUBST([GIFLIB_LIBS],["${found_lib}"])],
                  [i_do_have_giflib=no])
fi
dnl version 5+ breaks basic interface, so must use enhanced "DGifOpenFileName"
if test x"${i_do_have_giflib}" = xyes -a x"${GIFLIB_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([EGifGetGifVersion],[gif ungif],
                  [AC_SUBST([_GIFLIB_5PLUS],["${found_lib}"])],[])
fi
if test x"${i_do_have_giflib}" = xyes -a x"${GIFLIB_CFLAGS}" = x; then
   AC_CHECK_HEADER(gif_lib.h,[AC_SUBST([GIFLIB_CFLAGS],[""])],[i_do_have_giflib=no])
   if test x"${i_do_have_giflib}" = xyes; then
      AC_CACHE_CHECK([for ExtensionBlock.Function in gif_lib.h],
        ac_cv_extensionblock_in_giflib,
        [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <gif_lib.h>],[ExtensionBlock foo; foo.Function=3;])],
          [ac_cv_extensionblock_in_giflib=yes],[ac_cv_extensionblock_in_giflib=no])])
      if test x"${ac_cv_extensionblock_in_giflib}" != xyes; then
         AC_MSG_WARN([FontForge found giflib or libungif but cannot use this version. We will build without it.])
         i_do_have_giflib=no
      fi
   fi
fi
if test x"${i_do_have_giflib}" != xyes; then
   FONTFORGE_WARN_PKG_NOT_FOUND([GIFLIB])
   AC_DEFINE([_NO_LIBUNGIF],1,[Define if not using giflib or libungif.)])
fi
])

dnl There is no pkg-config support for libjpeg, at least on Gentoo. (17 Jul 2012)
dnl
dnl FONTFORGE_ARG_WITH_LIBJPEG
dnl --------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBJPEG],
[
AC_ARG_VAR([LIBJPEG_CFLAGS],[C compiler flags for LIBJPEG, overriding the automatic detection])
AC_ARG_VAR([LIBJPEG_LIBS],[linker flags for LIBJPEG, overriding the automatic detection])
AC_ARG_WITH([libjpeg],[AS_HELP_STRING([--without-libjpeg],[build without JPEG support])],
            [i_do_have_libjpeg="${withval}"],[i_do_have_libjpeg=yes])
if test x"${i_do_have_libjpeg}" = xyes -a x"${LIBJPEG_LIBS}" = x; then
   FONTFORGE_SEARCH_LIBS([jpeg_CreateDecompress],[jpeg],
                  [AC_SUBST([LIBJPEG_LIBS],["${found_lib}"])],
                  [i_do_have_libjpeg=no])
fi
if test x"${i_do_have_libjpeg}" = xyes -a x"${LIBJPEG_CFLAGS}" = x; then
   AC_CHECK_HEADER([jpeglib.h],[AC_SUBST([LIBJPEG_CFLAGS],[""])],[i_do_have_libjpeg=no])
fi
if test x"${i_do_have_libjpeg}" != xyes; then
   FONTFORGE_WARN_PKG_NOT_FOUND([LIBJPEG])
   AC_DEFINE([_NO_LIBJPEG],1,[Define if not using libjpeg.])
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

AC_DEFUN([CHECK_LIBUUID],
	[
	PKG_CHECK_MODULES([LIBUUID], [uuid >= 1.41.2], [LIBUUID_FOUND=yes], [LIBUUID_FOUND=no])
	if test "$LIBUUID_FOUND" = "no" ; then
	    PKG_CHECK_MODULES([LIBUUID], [uuid], [LIBUUID_FOUND=yes], [LIBUUID_FOUND=no])
	    if test "$LIBUUID_FOUND" = "no" ; then
                AC_MSG_ERROR([libuuid development files required])
            else
                LIBUUID_CFLAGS+=" -I$(pkg-config --variable=includedir uuid)/uuid "
            fi
	fi
	AC_SUBST([LIBUUID_CFLAGS])
	AC_SUBST([LIBUUID_LIBS])
	])

dnl FONTFORGE_ARG_WITH_ZEROMQ
dnl -------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_ZEROMQ],
[
FONTFORGE_ARG_WITH([libzmq],
        [AS_HELP_STRING([--without-libzmq],[build without libzmq])],
        [ libczmq >= 2.0.1 libzmq >= 4.0.0 ],
        [FONTFORGE_WARN_PKG_NOT_FOUND([LIBZMQ])],
        [_NO_LIBZMQ], [NO_LIBZMQ=1])
if test "x$i_do_have_libzmq" = xyes; then
   if test "x${WINDOWS_CROSS_COMPILE}" = x; then
      AC_MSG_WARN([Using zeromq enables collab, which needs libuuid, so I'm checking for that now...])
      CHECK_LIBUUID
   fi
fi

LIBZMQ_CFLAGS+=" $LIBUUID_CFLAGS"
LIBZMQ_LIBS+=" $LIBUUID_LIBS"

])
