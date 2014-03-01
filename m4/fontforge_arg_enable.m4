dnl -*- autoconf -*-

dnl FONTFORGE_ARG_ENABLE(option, help-message, config-entry)
dnl --------------------------------------------------------
AC_DEFUN([FONTFORGE_ARG_ENABLE],
[
AC_ARG_ENABLE([$1],[$2],
        [eval AS_TR_SH(i_do_have_$1)="${enableval}"],
        [eval AS_TR_SH(i_do_have_$1)=no])
if test x"${AS_TR_SH(i_do_have_$1)}" = xyes; then
   AC_DEFINE([$3],1,[Define if enabling feature '$1'.])
fi
])

dnl FONTFORGE_ARG_DISABLE(option, help-message, config-entry)
dnl ---------------------------------------------------------
AC_DEFUN([FONTFORGE_ARG_DISABLE],
[
AC_ARG_ENABLE([$1],[$2],
        [eval AS_TR_SH(i_do_have_$1)="${enableval}"],
        [eval AS_TR_SH(i_do_have_$1)=yes])
if test x"${AS_TR_SH(i_do_have_$1)}" != xyes; then
   AC_DEFINE([$3],1,[Define if disabling feature '$1'.])
fi
])

dnl FONTFORGE_ARG_ENABLE_REAL
dnl -------------------------
AC_DEFUN([FONTFORGE_ARG_ENABLE_REAL],
[
AC_ARG_ENABLE([real],
        [AS_HELP_STRING([--enable-real=TYPE],
                [TYPE is float or double;
                 sets the floating point type used internally [default=double]])],
        [my_real_type="${enableval}"],
        [my_real_type=double])
if test x"${my_real_type}" = x"double"; then
   AC_DEFINE([FONTFORGE_CONFIG_USE_DOUBLE],1,[Define if using 'double' precision.])
elif test x"${my_real_type}" != x"float"; then
   AC_MSG_ERROR([Floating point type '${my_real_type}' not recognized.])
fi   
])
