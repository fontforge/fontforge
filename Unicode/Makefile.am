include common.mk

lib_LTLIBRARIES = libgunicode.la

libgunicode_la_SOURCES = $(GUNICODE_C_SRCFILES) $(GUNICODE_H_SRCFILES)

libgunicode_la_CPPFLAGS = "-I$(top_builddir)/inc"	\
	"-I$(top_srcdir)/inc" $(MY_CFLAGS)

libgunicode_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GUNI_VERSION):$(GUNI_REVISION):$(GUNI_AGE)"
