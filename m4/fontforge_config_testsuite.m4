dnl -*- autoconf -*-

dnl FONTFORGE_CONFIG_TESTSUITE
dnl --------------------------
AC_DEFUN([FONTFORGE_CONFIG_TESTSUITE],
[
AC_CONFIG_FILES([tests/atlocal])
AC_CONFIG_FILES([tests/findoverlapbugs.py],[chmod +x tests/findoverlapbugs.py])
AC_CONFIG_FILES([tests/test0001.py],[chmod +x tests/test0001.py])
AC_CONFIG_FILES([tests/test0101.py],[chmod +x tests/test0101.py])
AC_CONFIG_FILES([tests/test1001.py],[chmod +x tests/test1001.py])
AC_CONFIG_FILES([tests/test1001a.py],[chmod +x tests/test1001a.py])
AC_CONFIG_FILES([tests/test1001b.py],[chmod +x tests/test1001b.py])
AC_CONFIG_FILES([tests/test1001c.py],[chmod +x tests/test1001c.py])
AC_CONFIG_FILES([tests/test1002.py],[chmod +x tests/test1002.py])
AC_CONFIG_FILES([tests/test1003.py],[chmod +x tests/test1003.py])
AC_CONFIG_FILES([tests/test1004.py],[chmod +x tests/test1004.py])
AC_CONFIG_FILES([tests/test1005.py],[chmod +x tests/test1005.py])
AC_CONFIG_FILES([tests/test1006.py],[chmod +x tests/test1006.py])
AC_CONFIG_FILES([tests/test1007.py],[chmod +x tests/test1007.py])
AC_SUBST([i_do_have_python_scripting])
])
