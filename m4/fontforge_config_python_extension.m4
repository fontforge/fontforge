dnl -*- autoconf -*-

dnl FONTFORGE_CONFIG_PYTHON_EXTENSION
dnl ---------------------------------
AC_DEFUN([FONTFORGE_CONFIG_PYTHON_EXTENSION],
[
# Configure the Python extension.
#
# The Python extension has its own configure script, so it can be
# configured for an already installed libfontforge. Here, where we
# want the extension configured for the libfontforge we are making
# ourselves, the pkg-config settings for "libfontforge" are
# overridden.

if test x"${i_do_have_python_extension}" = xyes; then
   export LIBFONTFORGE_CFLAGS
   LIBFONTFORGE_CFLAGS="AS_ESCAPE([-I"$(top_srcdir)/../inc"])"

   export LIBFONTFORGE_LIBS
   LIBFONTFORGE_LIBS="AS_ESCAPE(["$(top_builddir)/../fontforge/libfontforge.la"])"

   AC_CONFIG_SUBDIRS([pyhook])
fi
])
