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


dnl FONTFORGE_ARG_WITH_LIBNAMESLIST
dnl -------------------------------
dnl Check if libuninameslist exists, and if yes, then also see if it has 3 newer functions in it
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNINAMESLIST],
[
   FONTFORGE_ARG_WITH_BASE([libuninameslist],
      [AS_HELP_STRING([--without-libuninameslist],[build without Unicode Name or Annotation support])],
      [libuninameslist],
      [FONTFORGE_WARN_PKG_NOT_FOUND([LIBUNINAMESLIST])],
      [_NO_LIBUNINAMESLIST],
      [
       FONTFORGE_SEARCH_LIBS([UnicodeNameAnnot],[uninameslist],
         [i_do_have_libuninameslist=yes
          AC_SUBST([LIBUNINAMESLIST_CFLAGS],[""])
          AC_SUBST([LIBUNINAMESLIST_LIBS],["${found_lib}"])
          AC_CHECK_FUNC([uniNamesList_NamesListVersion],[AC_DEFINE([_LIBUNINAMESLIST_FUN],
                        [1],[Libuninameslist library has 3 new functions.])])],
         [i_do_have_libuninameslist=no])
       ])
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


dnl FONTFORGE_ARG_WITH_LIBXML
dnl -------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBXML],
[
FONTFORGE_ARG_WITH([libxml],
        [AS_HELP_STRING([--without-libxml],[build without libxml2])],
        [libxml-2.0],
        [FONTFORGE_WARN_PKG_NOT_FOUND([LIBXML])],
        [_NO_LIBXML])
])

dnl FONTFORGE_ARG_WITH_LIBREADLINE
dnl --------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBREADLINE],
[
AC_ARG_VAR([LIBREADLINE_CFLAGS],[C compiler flags for LIBREADLINE, overriding the automatic detection])
AC_ARG_VAR([LIBREADLINE_LIBS],[linker flags for LIBREADLINE, overriding the automatic detection])
AC_ARG_WITH([libreadline],[AS_HELP_STRING([--without-libreadline],[build without READLINE support])],
            [i_do_have_libreadline="${withval}"],[i_do_have_libreadline=yes])
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
   AC_CHECK_HEADER([readline/readline.h],[AC_SUBST([LIBREADLINE_CFLAGS],[""])],[i_do_have_libreadline=no])
fi
if test x"${i_do_have_libreadline}" = xyes; then
   if test x"${LIBREADLINE_LIBS}" != x; then
      AC_SUBST([LIBREADLINE_LIBS],["${LIBREADLINE_LIBS}"])
   fi
else
   FONTFORGE_WARN_PKG_NOT_FOUND([LIBREADLINE])
   AC_DEFINE([_NO_LIBREADLINE],1,[Define if not using libreadline.])
fi
])

dnl A macro that will not be needed if we can count on libspiro
dnl having a pkg-config file. 
dnl
dnl FONTFORGE_ARG_WITH_LIBSPIRO
dnl ---------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBSPIRO],
[
FONTFORGE_ARG_WITH_BASE([libspiro],
   [AS_HELP_STRING([--without-libspiro],[build without support for Spiro contours])],
   [libspiro],
   [FONTFORGE_WARN_PKG_NOT_FOUND([LIBSPIRO])],
   [_NO_LIBSPIRO],
   [
      FONTFORGE_SEARCH_LIBS([TaggedSpiroCPsToBezier],[spiro],
         [i_do_have_libspiro=yes
          AC_SUBST([LIBSPIRO_CFLAGS],[""])
          AC_SUBST([LIBSPIRO_LIBS],["${found_lib}"])
          FONTFORGE_WARN_PKG_FALLBACK([LIBSPIRO])
          AC_CHECK_FUNC([TaggedSpiroCPsToBezier0],[AC_DEFINE([_LIBSPIRO_FUN],
                        [1],[Libspiro returns true or false.])])],
         [i_do_have_libspiro=no])
   ])
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


dnl FONTFORGE_ARG_WITH_ICONV
dnl ------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_ICONV],
[
AC_ARG_WITH([iconv],
   [AS_HELP_STRING([--without-iconv],[build without the system's iconv(3); use fontforge's instead])],
   [i_do_want_iconv="${withval}"],
   [i_do_want_iconv=yes]
)])


dnl FONTFORGE_WARN_PKG_NOT_FOUND
dnl ----------------------------
AC_DEFUN([FONTFORGE_WARN_PKG_NOT_FOUND],
   [AC_MSG_WARN([$1 was not found. We will continue building without it.])])


dnl FONTFORGE_WARN_PKG_FALLBACK
dnl ---------------------------
AC_DEFUN([FONTFORGE_WARN_PKG_FALLBACK],
   [AC_MSG_WARN([No pkg-config file was found for $1, but the library is present and we will try to use it.])])


dnl FONTFORGE_ARG_WITH_ZEROMQ
dnl -------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_ZEROMQ],
[
FONTFORGE_ARG_WITH([libzmq],
        [AS_HELP_STRING([--without-libzmq],[build without libzmq])],
        [ libczmq libzmq > 3.2.0 ],
        [FONTFORGE_WARN_PKG_NOT_FOUND([LIBZMQ])],
        [_NO_LIBZMQ])
])
