include common.mk

lib_LTLIBRARIES = libgutils.la libgioftp.la

libgutils_la_SOURCES = ${GUTILS_C_SRCFILES} ${GUTILS_H_SRCFILES}
libgioftp_la_SOURCES = ${GIOFTP_C_SRCFILES} ${GIOFTP_H_SRCFILES}

libgutils_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)
libgioftp_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

libgutils_la_LIBADD = $(top_builddir)/Unicode/libgunicode.la

libgutils_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GUTL_VERSION):$(GUTL_REVISION):$(GUTL_AGE)"
libgioftp_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(GFTP_VERSION):$(GFTP_REVISION):$(GFTP_AGE)"
