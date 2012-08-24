plugindir = ${pkglibdir}/plugins

AM_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"		\
	"-I$(top_builddir)/fontforge" "-I$(top_srcdir)/fontforge"	\
	$(MY_CFLAGS)

plugin_LTLIBRARIES =

#--------------------------------------------------------------------------
#
# Encoding plugins.

if PLUGIN_GB12345
plugin_LTLIBRARIES += gb12345.la
gb12345_la_SOURCES = gb12345.c
gb12345_la_LDFLAGS = -module -avoid-version
endif PLUGIN_GB12345

#--------------------------------------------------------------------------
#
# Plugins internal to the design of FontForge.

if PLUGGIFLIB
plugin_LTLIBRARIES += pluggiflib.la
pluggiflib_la_SOURCES = plug.c
pluggiflib_la_LDFLAGS = -module -avoid-version
pluggiflib_la_LIBADD = $(GIFLIB_LIBS)
endif PLUGGIFLIB

if PLUGLIBJPEG
plugin_LTLIBRARIES += pluglibjpeg.la
pluglibjpeg_la_SOURCES = plug.c
pluglibjpeg_la_LDFLAGS = -module -avoid-version
pluglibjpeg_la_LIBADD = $(LIBJPEG_LIBS)
endif PLUGLIBJPEG

if PLUGLIBPNG
plugin_LTLIBRARIES += pluglibpng.la
pluglibpng_la_SOURCES = plug.c
pluglibpng_la_LDFLAGS = -module -avoid-version
pluglibpng_la_LIBADD = $(LIBPNG_LIBS)
endif PLUGLIBPNG

if PLUGLIBTIFF
plugin_LTLIBRARIES += pluglibtiff.la
pluglibtiff_la_SOURCES = plug.c
pluglibtiff_la_LDFLAGS = -module -avoid-version
pluglibtiff_la_LIBADD = $(LIBTIFF_LIBS)
endif PLUGLIBTIFF

if PLUGLIBXML
plugin_LTLIBRARIES += pluglibxml.la
pluglibxml_la_SOURCES = plug.c
pluglibxml_la_LDFLAGS = -module -avoid-version
pluglibxml_la_LIBADD = $(LIBXML_LIBS)
endif PLUGLIBXML

if PLUGLIBUNINAMESLIST
plugin_LTLIBRARIES += pluglibuninameslist.la
pluglibuninameslist_la_SOURCES = plug.c
pluglibuninameslist_la_LDFLAGS = -module -avoid-version
pluglibuninameslist_la_LIBADD = $(LIBUNINAMESLIST_LIBS)
endif PLUGLIBUNINAMESLIST

if PLUGLIBUNINAMESLISTFR
plugin_LTLIBRARIES += pluglibuninameslistfr.la
pluglibuninameslistfr_la_SOURCES = plug.c
pluglibuninameslistfr_la_LDFLAGS = -module -avoid-version
pluglibuninameslistfr_la_LIBADD = $(LIBUNINAMESLIST_FR_LIBS)
endif PLUGLIBUNINAMESLISTFR

if PLUGLIBSPIRO
plugin_LTLIBRARIES += pluglibspiro.la
pluglibspiro_la_SOURCES = plug.c
pluglibspiro_la_LDFLAGS = -module -avoid-version
pluglibspiro_la_LIBADD = $(LIBSPIRO_LIBS)
endif PLUGLIBSPIRO

#--------------------------------------------------------------------------
