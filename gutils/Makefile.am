lib_LTLIBRARIES = libgutils.la libgioftp.la

#--------------------------------------------------------------------------

libgutils_la_SOURCES = divisors.c fsys.c gcol.c gimage.c				\
	gimagereadbmp.c gimageread.c gimagereadgif.c gimagereadjpeg.c		\
	gimagereadpng.c gimagereadras.c gimagereadrgb.c gimagereadtiff.c	\
	gimagereadxbm.c gimagereadxpm.c gimagewritebmp.c					\
	gimagewritegimage.c gimagewritejpeg.c gimagewritepng.c				\
	gimagewritexbm.c gimagewritexpm.c gwwintl.c gio.c giofile.c			\
	giohosts.c giomime.c giothread.c giotrans.c gimagebmpP.h			\
	giofuncP.h gioP.h

libgutils_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

libgutils_la_LIBADD = $(top_builddir)/Unicode/libgunicode.la

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libgutils_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -release $(VERSION)

#--------------------------------------------------------------------------

libgioftp_la_SOURCES = gioftp.c gioftpP.h
libgioftp_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libgioftp_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -release $(VERSION)

#--------------------------------------------------------------------------
