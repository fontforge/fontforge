dnl -*- autoconf -*-

dnl FONTFORGE_CREATE_PKGCONFIG_FILES
dnl --------------------------------
AC_DEFUN([FONTFORGE_CREATE_PKGCONFIG_FILES],
[
# Pkg-config does not like having letters in the version. We will just
# strip off "alpha", "beta", "pre", or similar designations.
__cleaned_version="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"

PKG_CHECK_EXISTS([libtiff-4],
        [__have_libtiff_pkg_config=yes],
        [__have_libtiff_pkg_config=no])

__pkg_deps=""
test x"${i_do_have_python_scripting}" = xyes && __pkg_deps="${__pkg_deps} python-${PYTHON_VERSION}"
test x"${i_do_have_libpng}" = xyes && __pkg_deps="${__pkg_deps} libpng"
test x"${i_do_have_libtiff}" = xyes -a x"${__have_libtiff_pkg_config}" = xyes && __pkg_deps="${__pkg_deps} libtiff-4"
test x"${i_do_have_libxml}" = xyes && __pkg_deps="${__pkg_deps} libxml-2.0"
test x"${i_do_have_libunicodenames}" = xyes && __pkg_deps="${__pkg_deps} libunicodenames"
test x"${i_do_have_cairo}" = xyes && __pkg_deps="${__pkg_deps} cairo"
test x"${i_do_have_cairo}" = xyes && my_libs="${my_libs} pangocairo"
test x"${i_do_have_gui}" = xyes && __pkg_deps="${__pkg_deps} pango"
test x"${i_do_have_gui}" = xyes && my_libs="${my_libs} pangoxft"
test x"${i_do_have_freetype}" = xyes && __pkg_deps="${__pkg_deps} freetype2"
__pkg_deps="${__pkg_deps} zlib"

__private_deps=""
__private_deps="${__private_deps} -lgioftp"
__private_deps="${__private_deps} -lgutils"
__private_deps="${__private_deps} -lgunicode"
test x"${i_do_have_tifflib}" = xyes -a x"${__have_libtiff_pkg_config}" != xyes  && __private_deps="${__private_deps} ${LIBTIFF_LIBS}"
test x"${i_do_have_giflib}" = xyes && __private_deps="${__private_deps} ${GIFLIB_LIBS}"
test x"${i_do_have_libjpeg}" = xyes && __private_deps="${__private_deps} ${LIBJPEG_LIBS}"
test x"${i_do_have_libspiro}" = xyes && __private_deps="${__private_deps} ${LIBSPIRO_LIBS}"
test x"${i_do_have_x}" = xyes && __private_deps="${__private_deps} ${X_PRE_LIBS} ${X_LIBS} ${X_EXTRA_LIBS}"
__private_deps="${__private_deps} ${PTHREAD_LIBS}"
__private_deps="${__private_deps} ${LIBLTDL}"
__private_deps="${__private_deps} ${LIBS}"

__private_exe_deps=""
test x"${i_do_have_gui}" = xyes && __private_exe_deps="${__private_exe_deps} -lgdraw"

AC_SUBST([LIBFONTFORGE_PKGCONFIG_VERSION],["${__cleaned_version}"])
AC_SUBST([LIBFONTFORGE_PKGCONFIG_REQUIRES],[])
AC_SUBST([LIBFONTFORGE_PKGCONFIG_REQUIRES_PRIVATE],["${__pkg_deps}"])
AC_SUBST([LIBFONTFORGE_PKGCONFIG_LIBS],["-L${libdir} -lfontforge"])
AC_SUBST([LIBFONTFORGE_PKGCONFIG_LIBS_PRIVATE],["${__private_deps}"])

AC_SUBST([LIBFONTFORGEEXE_PKGCONFIG_VERSION],["${__cleaned_version}"])
AC_SUBST([LIBFONTFORGEEXE_PKGCONFIG_REQUIRES],[])
AC_SUBST([LIBFONTFORGEEXE_PKGCONFIG_REQUIRES_PRIVATE],["libfontforge ${__pkg_deps}"])
AC_SUBST([LIBFONTFORGEEXE_PKGCONFIG_LIBS],["-L${libdir} -lfontforgeexe"])
AC_SUBST([LIBFONTFORGEEXE_PKGCONFIG_LIBS_PRIVATE],["-L${libdir} ${__private_exe_deps} ${__private_deps}"])

AC_CONFIG_FILES([libfontforge.pc])
AC_CONFIG_FILES([libfontforgeexe.pc])
])
