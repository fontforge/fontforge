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


dnl FONTFORGE_ARG_DISABLE_PYTHON_SCRIPTING_AND_EXTENSION
dnl ----------------------------------------------------
dnl Add Python scripting by default and also add python extensions too.
dnl If user says nothing, then assume yes unless python is not available.
dnl First check if python and developer files are available, if yes, then
dnl continue, however, if user indicates yes, then we must verify that we
dnl do have python and developer files available, otherwise halt and let
dnl user know that they must add python and python developer files too.
dnl This is setup to try v3.3+ first, then v2.7+ if user requests nothing
dnl or yes or A. If user request 2 or 3, then we check for 2.7+ or 3.3+
AC_DEFUN([FONTFORGE_ARG_DISABLE_PYTHON_SCRIPTING_AND_EXTENSION],
[
i_want_python_ver="A"
AC_ARG_ENABLE([python-scripting],
        [AS_HELP_STRING([--disable-python-scripting],[disable Python scripting. If you
        REQUIRE Python, then use --enable-python-scripting to force a yes for install.
        If you want python>=3.3, then use --enable-python-scripting=3, use 2 if you
        want python>=2.7. yes will use 3.3+ or 2.7+ (whichever is available).])],
        [i_do_have_python_scripting="${enableval}"; enable_python_scripting="${enableval}"],
        [i_do_have_python_scripting=yes; enable_python_scripting=check])
AC_ARG_ENABLE([python-extension],
         [AS_HELP_STRING([--disable-python-extension],
                         [do not build the Python extension modules "psMat" and "fontforge",
                          even if they were included in this source distribution])],
         [i_do_have_python_extension="${enableval}"; enable_python_extension="${enableval}"],
         [i_do_have_python_extension=yes; enable_python_extension=check])
# default is to load python 3.3+ or 2.7+, but user forces a request for 2.7+ or 3.3+
if test x${i_do_have_python_scripting} = x2; then
   i_want_python_ver="2"
   i_do_have_python_scripting=yes
   enable_python_scripting=yes
else
   if test x${i_do_have_python_scripting} = x3; then
      i_want_python_ver="3"
      i_do_have_python_scripting=yes
      enable_python_scripting=yes
   else
      if test ${i_do_have_python_scripting}x = Ax; then
         i_want_python_ver="A"
         i_do_have_python_scripting=yes
         enable_python_scripting=yes
      else
         if test ${i_do_have_python_scripting}x = ax; then
            i_want_python_ver="A"
            i_do_have_python_scripting=yes
            enable_python_scripting=yes
         fi
      fi
   fi
fi
# force yes for python_scripting if user says yes for python_extension
if test x"${enable_python_extension}" = xyes; then
   i_do_have_python_scripting=yes
   if test x"${enable_python_scripting}" = xno; then
      enable_python_scripting=yes
   fi
fi
# first, test for python (we want defaults returned, avoid adding anything in yes section)
if test x"${i_do_have_python_scripting}" = xyes; then
   if test x${i_want_python_ver} = x3; then
      AM_PATH_PYTHON([3.3],,[i_do_have_python_scripting=no])
   else
      if test x${i_want_python_ver} = x2; then
         AM_PATH_PYTHON([2.7],,[i_do_have_python_scripting=no])
dnl TODO: verify this is actually 2.7+ and not 3.3+
dnl      if test x"${i_do_have_python_scripting}" = xyes; then
dnl         if PY_MAJOR_VERSION >= 3; then
dnl            i_do_have_python_scripting=no
dnl         fi
dnl      fi
      else
dnl      # default is try for 3.3+, otherwise try for 2.7+
dnl      AM_PATH_PYTHON([3.3],,[i_do_have_python_scripting=no])
dnl      if test x"${i_do_have_python_scripting}" != xyes; then
dnl         i_do_have_python_scripting=yes
            AM_PATH_PYTHON([2.7],,[i_do_have_python_scripting=no])
dnl         if test x"${i_do_have_python_scripting}" = xyes; then
dnl            i_want_python_ver="2" dnl only 2.7+ available
dnl         else
               i_want_python_ver=
dnl         fi
dnl      else
dnl         i_want_python_ver="3" might be 3 and 2 available
dnl      fi
      fi
   fi
fi
# second, test for python-dev
if test x"${i_do_have_python_scripting}" != xyes; then
   i_want_python_ver=
else
   PKG_CHECK_MODULES([PYTHON],[python-"${PYTHON_VERSION}"], dnl   [PKG_CHECK_MODULES([PYTHONDEV],[python-"${PYTHON_VERSION}"],,[i_do_have_python_scripting=maybe])],
      [PKG_CHECK_MODULES([PYTHONDEV],[python-"${PYTHON_VERSION}"],,[i_do_have_python_scripting=no])],
      [i_do_have_python_scripting=no])
dnl dnl TODO: have python3 AND python2, but only have python2 dev, but no python3 dev
dnl if test x"${i_do_have_python_scripting}" = xmaybe; then
dnl   if test x${i_want_python_ver} = xA; then
dnl      i_want_python_ver="2"
dnl      i_do_have_python_scripting=yes
dnl      AM_PATH_PYTHON([2.7],,[i_do_have_python_scripting=no])
dnl      if test x"${i_do_have_python_scripting}" = xyes; then
dnl         PKG_CHECK_MODULES([PYTHONDEV],[python-"${PYTHON_VERSION}"],,[i_do_have_python_scripting=no])
dnl      fi
dnl   else
dnl      i_do_have_python_scripting=no
dnl   fi
dnl   i_want_python_ver=
dnl fi
   if test x"${i_do_have_python_scripting}" = xno; then
      force_off_python_extension=yes
   fi

fi
# third, check for header file in case older distro package info not enough
if test x"${i_do_have_python_scripting}" = xyes -a x"${PYTHON_CFLAGS}" = x; then
   AC_CHECK_HEADER([python.h],[],[i_do_have_python_scripting=no])
fi
# force a halt if user specified yes, but there is no developer package found
AC_MSG_CHECKING([Build with Python support?])
if test x"${enable_python_scripting}" = xyes; then
   if test x"${i_do_have_python_scripting}" = xyes; then
      AC_MSG_RESULT([yes])
      i_want_python_ver="${PYTHON_VERSION}"
   else
      AC_MSG_FAILURE([ERROR: Please install the PYTHON Developer Package],[1])
   fi
else
   if test x"${i_do_have_python_scripting}" = xno || test x"${enable_python_scripting}" = xno; then
      AC_MSG_RESULT([no])
      i_do_have_python_scripting=no
      i_do_have_python_extension=no
      i_want_python_ver=
      AC_DEFINE([_NO_PYTHON],1,[Define if not using Python.])
      PYTHON_CFLAGS=""
      PYTHON_LIBS=""
   else
      AC_MSG_RESULT([yes])
      i_want_python_ver="${PYTHON_VERSION}"

   fi
fi
AC_MSG_CHECKING([Build with Python extension modules "psMat" and "fontforge"?])
if test x"${i_do_have_python_extension}" = xyes; then
   AC_MSG_RESULT([yes])
else
   AC_MSG_RESULT([no])
fi
AC_SUBST([PYTHON_CFLAGS])
AC_SUBST([PYTHON_LIBS])
AM_CONDITIONAL([PYTHON_SCRIPTING],[test x"${i_do_have_python_scripting}" = xyes])
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
dnl The default action  is to check for Xwindows and use it if available,
dnl but this can be overridden by using --enable-gdk to use gdk2 or gdk3.
dnl If no option {gdk2/gdk3} is specified, then the default is to try use
dnl gdk3. If no gdk development module is found then come to a hard stop.
AC_DEFUN([FONTFORGE_ARG_ENABLE_GDK],
[
fontforge_gdk_version=no
AC_ARG_ENABLE([gdk],
        [AS_HELP_STRING([--enable-gdk=TYPE],
                [Enable the GDK GUI backend. TYPE is either gdk2 or gdk3.])],
        [use_gdk=yes])
if test x$use_gdk = xyes ; then
    if test "x$enableval" = "xgdk2"; then
        PKG_CHECK_MODULES([GDK], [gdk-2.0 >= 2.10],
        [
            fontforge_can_use_gdk=yes
            fontforge_gdk_version=GDK2
            AC_DEFINE(FONTFORGE_CAN_USE_GDK,[],[FontForge will build the GUI with the GDK2 backend])
            AC_MSG_NOTICE([building the GUI with the GDK2 backend...])
        ],
        [
            AC_MSG_ERROR([Cannot build GDK backend without GDK installed. Please install the GTK+ Developer Package.])
        ])
    else
        PKG_CHECK_MODULES([GDK],[gdk-3.0 >= 3.10],
        [
            fontforge_can_use_gdk=yes
            fontforge_gdk_version=GDK3
            AC_DEFINE(FONTFORGE_CAN_USE_GDK,[],[FontForge will build the GUI with the GDK3 backend])
            AC_MSG_NOTICE([building the GUI with the GDK3 backend...])
        ],
        [
            AC_MSG_ERROR([Cannot build GDK backend without GDK installed. Please install the GTK+ Developer Package.])
        ])
    fi
else
    fontforge_can_use_gdk=no
fi
AM_CONDITIONAL([FONTFORGE_CAN_USE_GDK],[test x"${fontforge_can_use_gdk}" = xyes])
])

dnl FONTFORGE_ARG_ENABLE_WOFF2
dnl ------------------------
AC_DEFUN([FONTFORGE_ARG_ENABLE_WOFF2],
[
AC_ARG_ENABLE([woff2],
        [AS_HELP_STRING([--enable-woff2],
                [Enable WOFF2 support.])],
        [use_woff2=yes])
if test x$use_woff2 = xyes ; then
    PKG_CHECK_MODULES([WOFF2],[libwoff2enc,libwoff2dec],
    [
        AC_LANG_PUSH([C++])
        AX_CHECK_COMPILE_FLAG([-std=c++11],
            [CXXFLAGS="$CXXFLAGS -std=c++11"],
            [AC_MSG_ERROR([Get a compiler with C++11 support.])])
        AC_LANG_POP
        fontforge_can_use_woff2=yes
        fontforge_has_woff2=yes
        AC_DEFINE(FONTFORGE_CAN_USE_WOFF2,[],[FontForge will build with WOFF2 support])
        AC_MSG_NOTICE([building with WOFF2 support enabled...])
    ],
    [
        AC_MSG_ERROR([Cannot build with WOFF2 support without the woff2 development library installed.])
    ])
else
    fontforge_can_use_woff2=no
fi
AM_CONDITIONAL([FONTFORGE_CAN_USE_WOFF2],[test x"${fontforge_can_use_woff2}" = xyes])
])
