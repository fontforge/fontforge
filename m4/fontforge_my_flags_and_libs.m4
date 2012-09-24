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
my_cflags="${my_cflags} ${FREETYPE_CFLAGS}"
my_cflags="${my_cflags} ${GIFLIB_CFLAGS}"
my_cflags="${my_cflags} ${LIBJPEG_CFLAGS}"
my_cflags="${my_cflags} ${LIBPNG_CFLAGS}"
my_cflags="${my_cflags} ${LIBTIFF_CFLAGS}"
my_cflags="${my_cflags} ${LIBSPIRO_CFLAGS}"
my_cflags="${my_cflags} ${LIBUNICODENAMES_CFLAGS}"
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
test x"${i_do_have_python_scripting}" = xyes && my_libs="${my_libs} ${PYTHON_LIBS}"
test x"${i_do_have_giflib}" = xyes && my_libs="${my_libs} ${GIFLIB_LIBS}"
test x"${i_do_have_libjpeg}" = xyes && my_libs="${my_libs} ${LIBJPEG_LIBS}"
test x"${i_do_have_libpng}" = xyes && my_libs="${my_libs} ${LIBPNG_LIBS}"
test x"${i_do_have_libtiff}" = xyes && my_libs="${my_libs} ${LIBTIFF_LIBS}"
test x"${i_do_have_libunicodenames}" = xyes && my_libs="${my_libs} ${LIBUNICODENAMES_LIBS}"
test x"${i_do_have_libxml}" = xyes && my_libs="${my_libs} ${LIBXML_LIBS}"
test x"${i_do_have_libspiro}" = xyes && my_libs="${my_libs} ${LIBSPIRO_LIBS}"
test x"${i_do_have_cairo}" = xyes && my_libs="${my_libs} ${CAIRO_LIBS}"
test x"${i_do_have_cairo}" = xyes && my_libs="${my_libs} ${PANGOCAIRO_LIBS}"
test x"${i_do_have_freetype}" = xyes && my_libs="${my_libs} ${FREETYPE_LIBS}"
test x"${i_do_have_gui}" = xyes && my_libs="${my_libs} ${PANGO_LIBS}"
test x"${i_do_have_x}" = xyes && my_libs="${my_libs} ${X_PRE_LIBS} ${X_LIBS} ${X_EXTRA_LIBS}"
my_libs="${my_libs} ${PTHREAD_LIBS}"
my_libs="${my_libs} ${ZLIB_LIBS}"
my_libs="${my_libs} ${LIBLTDL}"
AC_SUBST([MY_LIBS],[${my_libs}])
])
