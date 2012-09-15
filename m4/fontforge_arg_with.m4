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
])


dnl Call this if you want to use pkg-config and don't have a package-specific hack.
dnl
dnl FONTFORGE_ARG_WITH(package, help-message, pkg-config-name, not-found-warning-message, _NO_XXXXX)
dnl ------------------------------------------------------------------------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH],
   [FONTFORGE_ARG_WITH_BASE([$1],[$2],[$3],[$4],[$5],[eval AS_TR_SH(i_do_have_$1)=no])])


dnl FONTFORGE_ARG_WITH_LIBUNICODENAMES
dnl ----------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNICODENAMES],
[
   FONTFORGE_ARG_WITH([libunicodenames],
      [AS_HELP_STRING([--without-libunicodenames],
                      [do not include access to Unicode NamesList data])],
      [libunicodenames],
      [FONTFORGE_WARN_PKG_NOT_FOUND([LIBUNICODENAMES])],
      [_NO_LIBUNICODENAMES])
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


dnl FONTFORGE_ARG_WITH_FREETYPE
dnl ---------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_FREETYPE],
[
FONTFORGE_ARG_WITH([freetype],
        [AS_HELP_STRING([--without-freetype],[build without freetype2])],
        [freetype2],
        [FONTFORGE_WARN_PKG_NOT_FOUND([FREETYPE])],
        [_NO_FREETYPE])
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
          FONTFORGE_WARN_PKG_FALLBACK([LIBSPIRO])],
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
