plugindir = ${pkglibdir}/plugins

AM_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"		\
	"-I$(top_builddir)/fontforge" "-I$(top_srcdir)/fontforge"	\
	$(MY_CFLAGS)

if PLUGIN_GB12345
plugin_LTLIBRARIES = gb12345.la
gb12345_la_SOURCES = gb12345.c
gb12345_la_LDFLAGS = -module -avoid-version
gb12345_la_LIBADD  = $(top_builddir)/fontforge/libfontforge.la
endif PLUGIN_GB12345
