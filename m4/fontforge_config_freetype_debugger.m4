dnl -*- autoconf -*-

dnl FONTFORGE_CONFIG_FREETYPE_DEBUGGER
dnl ----------------------------------
AC_DEFUN([FONTFORGE_CONFIG_FREETYPE_DEBUGGER],
[
if test x"${i_do_have_freetype_debugger}" = xyes; then
   i_do_have_freetype_debugger="${FREETYPE_SOURCE}"
elif test x"${i_do_have_freetype_debugger}" != xno; then
   FREETYPE_SOURCE="${i_do_have_freetype_debugger}"
fi
if test x"${i_do_have_freetype_debugger}" != xno; then
   if test ! -r "${FREETYPE_SOURCE}/src/truetype/ttobjs.h"; then
      AC_MSG_ERROR([

The file \"${FREETYPE_SOURCE}/src/truetype/ttobjs.h\"
was not found: \"${FREETYPE_SOURCE}\"
does not appear to be the top directory of the freetype
sources.

Please either set the environment variable FREETYPE_SOURCE
to the top directory of the freetype source tree, or
use the --disable-freetype-debugger option.

])

   fi
   AC_DEFINE([FREETYPE_HAS_DEBUGGER],[1],
        [Define if using the freetype debugger (requires freetype source code).])
fi
])
