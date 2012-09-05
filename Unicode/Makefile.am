lib_LTLIBRARIES = libgunicode.la

libgunicode_la_SOURCES = ArabicForms.c alphabet.c backtrns.c char.c		\
	 cjk.c memory.c ucharmap.c unialt.c ustring.c utype.c usprintf.c	\
	 gwwiconv.c combiners.h

libgunicode_la_CPPFLAGS = "-I$(top_builddir)/inc"	\
	"-I$(top_srcdir)/inc" $(MY_CFLAGS)

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libgunicode_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -release $(VERSION)
