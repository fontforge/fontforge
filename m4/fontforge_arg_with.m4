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


dnl FONTFORGE_ARG_WITH_CAIRO
dnl ------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_CAIRO],
[
   FONTFORGE_ARG_WITH([cairo],
      [AS_HELP_STRING([--without-cairo],
                      [build without Cairo graphics (use regular X graphics instead)])],
      [cairo],
      [FONTFORGE_WARN_PKG_NOT_FOUND([CAIRO])],
      [_NO_LIBCAIRO])
])


dnl FONTFORGE_ARG_WITH_PANGO
dnl ------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_PANGO],
[
   FONTFORGE_ARG_WITH([pango],
      [AS_HELP_STRING([--without-pango],
          [build without Pango text rendering (use less sophisticated rendering instead)])],
      [pango],
      [FONTFORGE_WARN_PKG_NOT_FOUND([PANGO])],
      [_NO_LIBPANGO])
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


dnl A macro that will not be needed if we can count on libuninameslist
dnl having a pkg-config file. 
dnl
dnl FONTFORGE_ARG_WITH_LIBUNINAMESLIST
dnl ----------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNINAMESLIST],
[
   FONTFORGE_ARG_WITH_BASE([libuninameslist],
      [AS_HELP_STRING([--without-libuninameslist],[build without libuninameslist])],
      [libuninameslist],
      [FONTFORGE_WARN_PKG_NOT_FOUND([LIBUNINAMESLIST])],
      [_NO_LIBUNINAMESLIST],
      [FONTFORGE_ARG_WITH_LIBUNINAMESLIST_fallback])
])
dnl
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNINAMESLIST_fallback],
[
   FONTFORGE_SEARCH_LIBS([UnicodeNameAnnot],[uninameslist],
      [i_do_have_libuninameslist=yes
       AC_SUBST([LIBUNINAMESLIST_CFLAGS],[""])
       AC_SUBST([LIBUNINAMESLIST_LIBS],["${found_lib}"])
       FONTFORGE_WARN_PKG_FALLBACK([LIBUNINAMESLIST])],
      [i_do_have_libuninameslist=no])
])

dnl A macro that will not be needed if we can count on libuninameslist-fr
dnl having a pkg-config file. 
dnl
dnl FONTFORGE_ARG_WITH_LIBUNINAMESLIST_FR
dnl -------------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNINAMESLIST_FR],
[
   FONTFORGE_ARG_WITH_BASE([libuninameslistfr],
      [AS_HELP_STRING([--without-libuninameslist-fr],[build without libuninameslist-fr])],
      [libuninameslist-fr],
      [FONTFORGE_WARN_PKG_NOT_FOUND([LIBUNINAMESLISTFR])],
      [_NO_LIBUNINAMESLISTFR],
      [FONTFORGE_ARG_WITH_LIBUNINAMESLISTFR_fallback])
])
dnl
AC_DEFUN([FONTFORGE_ARG_WITH_LIBUNINAMESLISTFR_fallback],
[
   FONTFORGE_SEARCH_LIBS([UnicodeNameAnnot],[uninameslist-fr],
      [i_do_have_libuninameslistfr=yes
       AC_SUBST([LIBUNINAMESLISTFR_CFLAGS],[""])
       AC_SUBST([LIBUNINAMESLISTFR_LIBS],["${found_lib}"])
       FONTFORGE_WARN_PKG_FALLBACK([LIBUNINAMESLISTFR])],
      [i_do_have_libuninameslistfr=no])
])

dnl A macro that will not be needed if we can count on libspiro
dnl having a pkg-config file. 
dnl
dnl FONTFORGE_ARG_WITH_LIBSPIRO
dnl ---------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_LIBSPIRO],
[
FONTFORGE_ARG_WITH_BASE([libspiro],
   [AS_HELP_STRING([--without-libspiro],[build without support for Spiro])],
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


dnl FONTFORGE_ARG_WITH_REGULAR_LINK
dnl ---------------------------------
AC_DEFUN([FONTFORGE_ARG_WITH_REGULAR_LINK],
[
AC_ARG_WITH([regular-link],
        [AS_HELP_STRING([--with-regular-link[[=PKGS]]],
                [use regular linking instead of dlopen, optionally listing individual packages to link
                 without dlopen;
                 PKGS can include
                    cairo,
                    pango,
                    freetype,
                    giflib,
                    libjpeg,
                    libpng,
                    libtiff,
                    libxml,
                    libuninameslist,
                    libspiro,
                    libintl])],
        [i_do_have_regular_link="${withval}"],
        [i_do_have_regular_link=no])

if test x"${i_do_have_regular_link}" = xyes; then
   AC_DEFINE([NODYNAMIC],1,[Define if enabling feature 'regular-link' for all packages.])
   regular_link_giflib=yes
   regular_link_libjpeg=yes
   regular_link_libpng=yes
   regular_link_libtiff=yes
   regular_link_freetype=yes
   regular_link_libuninameslist=yes
   regular_link_libxml=yes
   regular_link_libspiro=yes
   regular_link_cairo=yes
   regular_link_pango=yes
   regular_link_libintl=yes
fi

if test x"${i_do_have_regular_link}" != xyes -a x"${i_do_have_regular_link}" != xno; then
   for _my_pkg in `AS_ECHO(["${i_do_have_regular_link}"]) | tr ',' ' '`; do
       AS_CASE(["${_my_pkg}"],
        [giflib],[AC_DEFINE([_STATIC_LIBUNGIF],[1],[Define if enabling feature 'regular-link' for giflib.])
                  regular_link_giflib=yes],
        [libjpeg],[AC_DEFINE([_STATIC_LIBJPEG],[1],[Define if enabling feature 'regular-link' for libjpeg.])
                   regular_link_libjpeg=yes],
        [libpng],[AC_DEFINE([_STATIC_LIBPNG],[1],[Define if enabling feature 'regular-link' for libpng.])
                  regular_link_libpng=yes],
        [libtiff],[AC_DEFINE([_STATIC_LIBTIFF],[1],[Define if enabling feature 'regular-link' for libtiff.])
                   regular_link_libtiff=yes],
        [freetype],[AC_DEFINE([_STATIC_LIBFREETYPE],[1],[Define if enabling feature 'regular-link' for freetype.])
                    regular_link_freetype=yes],
        [libuninameslist],[AC_DEFINE([_STATIC_LIBUNINAMESLIST],[1],[Define if enabling feature 'regular-link' for libuninameslist.])
                  regular_link_libuninameslist=yes],
        [libxml],[AC_DEFINE([_STATIC_LIBXML],[1],[Define if enabling feature 'regular-link' for libxml.])
                  regular_link_libxml=yes],
        [libspiro],[AC_DEFINE([_STATIC_LIBSPIRO],[1],[Define if enabling feature 'regular-link' for libspiro.])
                    regular_link_libspiro=yes],
        [cairo],[AC_DEFINE([_STATIC_LIBCAIRO],[1],[Define if enabling feature 'regular-link' for cairo.])
                 regular_link_cairo=yes],
        [pango],[AC_DEFINE([_STATIC_LIBPANGO],[1],[Define if enabling feature 'regular-link' for pango.])
                 regular_link_pango=yes],
        [libintl],[AC_DEFINE([_STATIC_LIBINTL],[1],[Define if enabling feature 'regular-link' for libintl.])
                   regular_link_libintl=yes],
        [:] dnl FIXME: Give a warning in the default case.
       )       
   done
fi

if test x"${i_do_have_regular_link}" = xyes -o x"${regular_link_libintl}" = xyes; then
   AC_SEARCH_LIBS([bindtextdomain],[intl])
fi

# glibc has the libintl.h functions in -lc. Therefore libintl.so need
# not and typically cannot be dynamically loaded on GNU systems and
# regular-link must be forced.
if test x"${regular_link_libintl}" != xyes; then
   AC_SEARCH_LIBS([bindtextdomain],[])
   if test x"${ac_cv_search_bindtextdomain}" = x"none required"; then
      AC_DEFINE([_STATIC_LIBINTL],[1])
      regular_link_libintl=yes
   fi
fi
])


dnl FONTFORGE_ARG_WITH_ARCH
dnl -----------------------
dnl
dnl This was going to be a replacement for the "--with-arch*" options
dnl of the old configuration system, but if someone wants "-arch *" in
dnl their compiler options they can simply add it to CFLAGS or CC.
dnl
AC_DEFUN([FONTFORGE_ARG_WITH_ARCH],[])


dnl FONTFORGE_WARN_PKG_NOT_FOUND
dnl ----------------------------
AC_DEFUN([FONTFORGE_WARN_PKG_NOT_FOUND],
   [AC_MSG_WARN([$1 was not found. We will continue building without it.])])


dnl FONTFORGE_WARN_PKG_FALLBACK
dnl ---------------------------
AC_DEFUN([FONTFORGE_WARN_PKG_FALLBACK],
   [AC_MSG_WARN([No pkg-config file was found for $1, but the library is present and we will try to use it.])])


dnl local variables:
dnl mode: autoconf
dnl end:
