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


dnl FONTFORGE_ARG_DISABLE_PYTHON_SCRIPTING
dnl --------------------------------------
AC_DEFUN([FONTFORGE_ARG_DISABLE_PYTHON_SCRIPTING],
[
AC_ARG_ENABLE([python-scripting],
        [AS_HELP_STRING([--disable-python-scripting],[disable Python scripting])],
        [i_do_have_python_scripting="${enableval}"],
        [i_do_have_python_scripting=yes])
if test x"${i_do_have_python_scripting}" = xyes; then
   AM_PATH_PYTHON([2.7])
   PKG_CHECK_MODULES([PYTHON],[python-"${PYTHON_VERSION}"],,[i_do_have_python_scripting=no; force_off_python_extension=yes])
fi
if test x"${i_do_have_python_scripting}" != xyes; then
   AC_DEFINE([_NO_PYTHON],1,[Define if not using Python.])
fi
AM_CONDITIONAL([PYTHON_SCRIPTING],[test x"${i_do_have_python_scripting}" = xyes])
])


dnl FONTFORGE_ARG_DISABLE_PYTHON_EXTENSION
dnl --------------------------------------
AC_DEFUN([FONTFORGE_ARG_DISABLE_PYTHON_EXTENSION],
[
AC_ARG_ENABLE([python-extension],
         [AS_HELP_STRING([--disable-python-extension],
                         [do not build the Python extension modules "psMat" and "fontforge",
                          even if they were included in this source distribution])],
         [i_do_have_python_extension="${enableval}"],
         [i_do_have_python_extension=yes])
dnl don't try to make the module unless we have python
if test x"${force_off_python_extension}" = xyes; then
         i_do_have_python_extension=no
fi
AM_CONDITIONAL([PYTHON_EXTENSION],[test x"${i_do_have_python_extension}" = xyes])
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

dnl FONTFORGE_ARG_ENABLE_GDK
dnl ------------------------
AC_DEFUN([FONTFORGE_ARG_ENABLE_GDK],
[
AC_ARG_ENABLE([gdk],
        [AS_HELP_STRING([--enable-gdk],
                [Enable the GDK GUI backend])],
        [use_gdk=yes])
if test x$use_gdk = xyes ; then
    PKG_CHECK_MODULES([GDK],[gdk-3.0 >= 3.10],
    [
        fontforge_can_use_gdk=yes
        fontforge_gdk_version=GDK3
        AC_DEFINE(FONTFORGE_CAN_USE_GDK,[],[FontForge will build the GUI with the GDK3 backend])
        AC_MSG_NOTICE([building the GUI with the GDK3 backend...])
    ],
    [
        AC_MSG_WARN([GDK3 not found, trying with GDK2...])
        PKG_CHECK_MODULES([GDK], [gdk-2.0 >= 2.10],
        [
            fontforge_can_use_gdk=yes
            fontforge_gdk_version=GDK2
            AC_DEFINE(FONTFORGE_CAN_USE_GDK,[],[FontForge will build the GUI with the GDK2 backend])
            AC_MSG_NOTICE([building the GUI with the GDK2 backend...])
        ],
        [
            fontforge_can_use_gdk=no
            AC_MSG_ERROR([Cannot build GDK backend without GDK installed.])
        ])
    ])
fi
AM_CONDITIONAL([FONTFORGE_CAN_USE_GDK],[test x"${fontforge_can_use_gdk}" = xyes])
])
