dnl -*- autoconf -*-

dnl FONTFORGE_COMPILER_FLAGS
dnl ------------------------
dnl Check for and add usable compiler warnings and flags.
dnl Usable flags are added to value=$1 from list=$2.
AC_DEFUN([FONTFORGE_COMPILER_FLAGS],
[
$1=""
m4_foreach_w([__flag],[$2],
   [AX_CHECK_COMPILE_FLAG([__flag],[$1="[$]{$1} __flag"])])
])
