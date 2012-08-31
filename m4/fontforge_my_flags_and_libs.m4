dnl -*- autoconf -*-

dnl FONTFORGE_SET_MY_CFLAGS
dnl -----------------------
AC_DEFUN([FONTFORGE_SET_MY_CFLAGS],
[
my_cflags="${WARNING_CFLAGS}"
if test x"${i_do_have_freetype_debugger}" != xno; then
   my_cflags="${my_cflags} -I${FREETYPE_SOURCE}/src/truetype"
   my_cflags="${my_cflags} -I${FREETYPE_SOURCE}/include"
   my_cflags="${my_cflags} -I${FREETYPE_SOURCE}/include/freetype"
fi
my_cflags="${my_cflags} ${ZLIB_CFLAGS}"
my_cflags="${my_cflags} ${CAIRO_CFLAGS}"
my_cflags="${my_cflags} ${PANGO_CFLAGS}"
my_cflags="${my_cflags} ${PANGOCAIRO_CFLAGS}"
my_cflags="${my_cflags} ${PANGOXFT_CFLAGS}"
my_cflags="${my_cflags} ${FREETYPE_CFLAGS}"
my_cflags="${my_cflags} ${GIFLIB_CFLAGS}"
my_cflags="${my_cflags} ${LIBJPEG_CFLAGS}"
my_cflags="${my_cflags} ${LIBPNG_CFLAGS}"
my_cflags="${my_cflags} ${LIBTIFF_CFLAGS}"
my_cflags="${my_cflags} ${LIBSPIRO_CFLAGS}"
my_cflags="${my_cflags} ${LIBUNINAMESLIST_CFLAGS}"
my_cflags="${my_cflags} ${LIBXML_CFLAGS}"
my_cflags="${my_cflags} ${PYTHON_CFLAGS}"
my_cflags="${my_cflags} ${PTHREAD_CFLAGS}"
test x"${SDK}" = x || my_cflags="${my_cflags} -I${SDK}"
test x"${LTDLINCL}" = x || my_cflags="${my_cflags} -I${LTDLINCL}"
AC_SUBST([MY_CFLAGS],[${my_cflags}])
])

dnl FONTFORGE_SET_MY_LIBS
dnl ---------------------
AC_DEFUN([FONTFORGE_SET_MY_LIBS],
[
my_libs="${PYTHON_LIBS}"
my_libs="${my_libs} ${GIFLIB_LIBS}"
my_libs="${my_libs} ${LIBJPEG_LIBS}"
my_libs="${my_libs} ${LIBPNG_LIBS}"
my_libs="${my_libs} ${LIBTIFF_LIBS}"
my_libs="${my_libs} ${LIBUNINAMESLIST_LIBS}"
my_libs="${my_libs} ${LIBXML_LIBS}"
my_libs="${my_libs} ${LIBSPIRO_LIBS}"
my_libs="${my_libs} ${CAIRO_LIBS}"
my_libs="${my_libs} ${PANGO_LIBS}"
my_libs="${my_libs} ${PANGOCAIRO_LIBS}"
my_libs="${my_libs} ${PANGOXFT_LIBS}"
my_libs="${my_libs} ${FREETYPE_LIBS}"
my_libs="${my_libs} ${X_PRE_LIBS} ${X_LIBS} ${X_EXTRA_LIBS}"
my_libs="${my_libs} ${PTHREAD_LIBS}"
my_libs="${my_libs} ${ZLIB_LIBS}"
my_libs="${my_libs} ${LIBLTDL}"
AC_SUBST([MY_LIBS],[${my_libs}])
])
