ACLOCAL_AMFLAGS = -I m4

python_LTLIBRARIES = psMat.la fontforge.la

psMat_la_SOURCES = psMatpyhook.c loadfontforge.h
psMat_la_CPPFLAGS = -DLIBDIR='"$(libdir)"' $(PYTHON_CFLAGS)	\
	$(LIBFONTFORGE_CFLAGS)
psMat_la_LDFLAGS = -module -shared -avoid-version
psMat_la_LIBADD = $(LIBFONTFORGE_LIBS)

fontforge_la_SOURCES = fontforgepyhook.c loadfontforge.h
fontforge_la_CPPFLAGS = -DLIBDIR='"$(libdir)"' $(PYTHON_CFLAGS)	\
	$(LIBFONTFORGE_CFLAGS)
fontforge_la_LDFLAGS = -module -shared -avoid-version
fontforge_la_LIBADD = $(LIBFONTFORGE_LIBS)

EXTRA_DIST = LICENSE
