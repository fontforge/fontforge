GUNI_VERSION = 4
GUNI_REVISION = 0
GUNI_AGE = 0

lib_LTLIBRARIES = libgunicode.la

libgunicode_la_SOURCES = ArabicForms.c alphabet.c backtrns.c char.c		\
	 cjk.c memory.c ucharmap.c unialt.c ustring.c utype.c usprintf.c	\
	 gwwiconv.c combiners.h

libgunicode_la_CPPFLAGS = "-I$(top_builddir)/inc"	\
	"-I$(top_srcdir)/inc" $(MY_CFLAGS)

libgunicode_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GUNI_VERSION):$(GUNI_REVISION):$(GUNI_AGE)"
