dnl -*- autoconf -*-

dnl FONTFORGE_SEARCH_LIBS
dnl ---------------------
AC_DEFUN([FONTFORGE_SEARCH_LIBS],
[
   _save_LIBS="${LIBS}"
   found_lib=""
   AC_SEARCH_LIBS([$1],[$2],
                  [if test x"${ac_cv_search_$1}" != x"none required"; then
                     found_lib="${ac_cv_search_$1}"
                   fi
                   $3],
                  [$4],
                  [$5])
   LIBS="${_save_LIBS}"
])
