lib_LTLIBRARIES = libgutils.la libgioftp.la

#--------------------------------------------------------------------------

GUTL_VERSION = 2
GUTL_REVISION = 0
GUTL_AGE = 0

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

libgutils_la_LIBADD =											\
	$(top_builddir)/pluginloading/libfontforgepluginloading.la	\
	$(top_builddir)/Unicode/libgunicode.la

libgutils_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GUTL_VERSION):$(GUTL_REVISION):$(GUTL_AGE)"

#--------------------------------------------------------------------------

GFTP_VERSION = 2
GFTP_REVISION = 0
GFTP_AGE = 0

libgioftp_la_SOURCES = gioftp.c gioftpP.h
libgioftp_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)
libgioftp_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GFTP_VERSION):$(GFTP_REVISION):$(GFTP_AGE)"

#--------------------------------------------------------------------------
