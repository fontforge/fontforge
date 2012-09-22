dnl -*- autoconf -*-

dnl FONTFORGE_CHECK_LTDL_VERSION
dnl ----------------------------
AC_DEFUN([FONTFORGE_CHECK_LTDL_VERSION],
[
if test x"${with_included_ltdl}" != xyes; then
   save_CFLAGS="${CFLAGS}"
   save_LDFLAGS="${LDFLAGS}"
   CFLAGS="${CFLAGS} ${LTDLINCL}"
   LDFLAGS="${LDFLAGS} ${LIBLTDL}"
   # The lt_dladvise_init symbol was added with libtool-2.2.
   AC_CHECK_LIB([ltdl],[lt_dladvise_init],[],
                [AC_MSG_ERROR([installed libltdl is too old; consider using --with-included-ltdl])])
   LDFLAGS="${save_LDFLAGS}"
   CFLAGS="${save_CFLAGS}"
fi
])
