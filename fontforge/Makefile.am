include common.mk

#--------------------------------------------------------------------------

if THE_PROGRAMS

bin_PROGRAMS = fontforge
dist_man_MANS = fontforge.1

if NATIVE_SCRIPTING
nodist_bin_SCRIPTS = fontimage fontlint sfddiff
dist_man_MANS += fontimage.1 fontlint.1 sfddiff.1
endif NATIVE_SCRIPTING

endif THE_PROGRAMS

lib_LTLIBRARIES = libfontforge.la libfontforgeexe.la

#--------------------------------------------------------------------------

SUBDIRS = pixmaps

FFSCRIPT_HEADER = "\#!$(bindir)/fontforge -lang=ff"

fontimage: fontimage.pe
	echo "${FFSCRIPT_HEADER}" | cat - "${srcdir}/fontimage.pe" > fontimage
	chmod +x fontimage

fontlint: fontlint.pe
	echo "${FFSCRIPT_HEADER}" | cat - "${srcdir}/fontlint.pe" > fontlint
	chmod +x fontlint

sfddiff: sfddiff.pe
	echo "${FFSCRIPT_HEADER}" | cat - "${srcdir}/sfddiff.pe" > sfddiff
	chmod +x sfddiff

.pre.c:
	$(SED) -e '1,$$ s/REPLACE_ME_WITH_MAJOR_VERSION/$(FF_VERSION)/'  \
	       -e '1,$$ s/REPLACE_ME_WITH_MINOR_VERSION/$(FF_REVISION)/' \
	    < $< > $@

#--------------------------------------------------------------------------

noinst_HEADERS = $(FF_H_SRCFILES)
pkginclude_HEADERS = $(INST_FF_H_SRCFILES)

#--------------------------------------------------------------------------

if THE_PROGRAMS

fontforge_SOURCES = $(FF_C_SRCFILES)
fontforge_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)
fontforge_LDADD = libfontforgeexe.la
fontforge_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -export-dynamic

endif THE_PROGRAMS

#--------------------------------------------------------------------------

libfontforge_la_SOURCES        = $(LIBFF_C_SRCFILES)
nodist_libfontforge_la_SOURCES = $(NODIST_LIBFF_C_SRCFILES)
EXTRA_libfontforge_la_SOURCES  = $(EXTRA_LIBFF_C_SRCFILES)

libfontforge_la_CPPFLAGS = "-I$(top_builddir)/inc"					\
	"-I$(top_srcdir)/inc" "-DSHAREDIR=\"${pkgdatadir}\""			\
	"-DDOCDIR=\"${docdir}\"" "-DPLUGINDIR=\"${pkglibdir}/plugins\""	\
	$(MY_CFLAGS)

libfontforge_la_LIBADD = splinerefigure.lo		\
	$(top_builddir)/Unicode/libgunicode.la		\
	$(top_builddir)/gutils/libgutils.la			\
	$(top_builddir)/gutils/libgioftp.la
if GRAPHICAL_USER_INTERFACE
libfontforge_la_LIBADD += $(top_builddir)/gdraw/libgdraw.la	\
	$(XINPUT_LIBS) $(XKB_LIBS)
endif GRAPHICAL_USER_INTERFACE

libfontforge_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(FF_VERSION):$(FF_REVISION):$(FF_AGE)"

#--------------------------------------------------------------------------

if GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = $(LIBFFEXE_C_SRCFILES)

else !GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = startnoui.c

endif !GRAPHICAL_USER_INTERFACE

nodist_libfontforgeexe_la_SOURCES = $(NODIST_LIBFFEXE_C_SRCFILES)

libfontforgeexe_la_CPPFLAGS = $(libfontforge_la_CPPFLAGS)
libfontforgeexe_la_LIBADD = libfontforge.la $(LTDLDEPS)
libfontforgeexe_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(FF_VERSION):$(FF_REVISION):$(FF_AGE)"

#--------------------------------------------------------------------------

# In splinerefigure.c, the slight error introduced occasionally by
# optimization turns out to have disastrous effects. Compile it
# without optimization.
splinerefigure.lo: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(LIBTOOL) --mode=compile $(CC) -g -DHAVE_CONFIG_H -c -o	\
		splinerefigure.lo $(libfontforge_la_CPPFLAGS)			\
		$(srcdir)/splinerefigure.c

#--------------------------------------------------------------------------

EXTRA_DIST = fontimage.pe fontlint.pe sfddiff.pe libstamp.pre	\
	exelibstamp.pre fontimage.1 fontlint.1 sfddiff.1
MOSTLYCLEANFILES = fontimage fontlint sfddiff libstamp.c	\
	exelibstamp.c
