PL_VERSION = 1
PL_REVISION = 0
PL_AGE = 0

lib_LTLIBRARIES = libfontforgepluginloading.la

libfontforgepluginloading_la_SOURCES = pluginloading.c

libfontforgepluginloading_la_CPPFLAGS = "-I$(top_builddir)/inc"		\
	"-I$(top_srcdir)/inc" "-DPLUGINDIR=\"${pkglibdir}/plugins\""	\
	$(MY_CFLAGS)

libfontforgepluginloading_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS)	\
	-version-info "$(PL_VERSION):$(PL_REVISION):$(PL_AGE)"

