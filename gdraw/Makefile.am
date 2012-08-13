include common.mk

lib_LTLIBRARIES = libgdraw.la

libgdraw_la_SOURCES = $(GDRAW_C_SRCFILES) $(GDRAW_H_SRCFILES)

libgdraw_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

libgdraw_la_LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la

libgdraw_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GD_VERSION):$(GD_REVISION):$(GD_AGE)"
