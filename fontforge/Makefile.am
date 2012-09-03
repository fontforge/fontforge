#--------------------------------------------------------------------------

FF_VERSION = 2
FF_REVISION = 0
FF_AGE = 0

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

noinst_HEADERS = fontforgeui.h sftextfieldP.h configure-fontforge.h	\
	views.h is_LIGATURE.h
pkginclude_HEADERS = autowidth2.h configure-fontforge.h fontforge.h		\
	libffstamp.h psfont.h stemdb.h autowidth.h delta.h fontforgevw.h	\
	lookups.h savefont.h ttf.h baseviews.h fvmetrics.h mm.h				\
	scriptfuncs.h ttfinstrs.h edgelist2.h namehash.h scripting.h		\
	uiinterface.h bezctx_ff.h edgelist.h groups.h nonlineartrans.h		\
	sd.h unicoderange.h bitmapcontrol.h encoding.h ofl.h search.h		\
	usermenu.h fffreetype.h PfEd.h sfd1.h ffpython.h import.h			\
	plugins.h sflayoutP.h print.h splinefont.h annotations.h

#--------------------------------------------------------------------------

if THE_PROGRAMS

fontforge_SOURCES = main.c
fontforge_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)
fontforge_LDADD = libfontforgeexe.la
fontforge_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -export-dynamic

endif THE_PROGRAMS

#--------------------------------------------------------------------------

libfontforge_la_SOURCES = asmfpst.c autohint.c autosave.c autotrace.c	\
	autowidth.c bezctx_ff.c bitmapchar.c bitmapcontrol.c bvedit.c		\
	clipnoui.c crctab.c cvexport.c cvimages.c cvundoes.c dumpbdf.c		\
	dumppfa.c effects.c encoding.c featurefile.c fontviewbase.c			\
	freetype.c fvcomposite.c fvfonts.c fvimportbdf.c fvmetrics.c		\
	glyphcomp.c http.c ikarus.c lookups.c macbinary.c macenc.c			\
	mathconstants.c mm.c namelist.c nonlineartrans.c noprefs.c			\
	nouiutil.c nowakowskittfinstr.c ofl.c othersubrs.c palmfonts.c		\
	parsepdf.c parsepfa.c parsettfatt.c parsettfbmf.c parsettf.c		\
	parsettfvar.c plugins.c print.c psread.c pua.c python.c				\
	savefont.c scripting.c scstyles.c search.c sfd1.c sfd.c				\
	sflayout.c spiro.c splinechar.c splinefill.c splinefont.c			\
	splineorder2.c splineoverlap.c splinesaveafm.c splinesave.c			\
	splinestroke.c splineutil2.c splineutil.c start.c stemdb.c svg.c	\
	tottfaat.c tottfgpos.c tottf.c tottfvar.c ttfinstrs.c				\
	ttfspecial.c ufo.c unicoderange.c utils.c winfonts.c zapfnomen.c	\
	groups.c langfreq.c ftdelta.c autowidth2.c woff.c stamp.c			\
	activeinui.c is_LIGATURE.c
nodist_libfontforge_la_SOURCES = libstamp.c
EXTRA_libfontforge_la_SOURCES = splinerefigure.c

libfontforge_la_CPPFLAGS = "-I$(top_builddir)/inc"					\
	"-I$(top_srcdir)/inc" "-DSHAREDIR=\"${pkgdatadir}\""			\
	"-DDOCDIR=\"${docdir}\"" "-DPLUGINDIR=\"${pkglibdir}/plugins\""	\
	$(MY_CFLAGS)

libfontforge_la_LIBADD = splinerefigure.lo						\
	$(top_builddir)/pluginloading/libfontforgepluginloading.la	\
	$(top_builddir)/Unicode/libgunicode.la						\
	$(top_builddir)/gutils/libgutils.la							\
	$(top_builddir)/gutils/libgioftp.la

if GRAPHICAL_USER_INTERFACE
libfontforge_la_LIBADD += $(top_builddir)/gdraw/libgdraw.la	\
	$(XINPUT_LIBS) $(XKB_LIBS)
endif GRAPHICAL_USER_INTERFACE

if PLUGIN_GB12345
libfontforge_la_LIBADD += -dlopen $(top_builddir)/plugins/gb12345.la
endif PLUGIN_GB12345

libfontforge_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(FF_VERSION):$(FF_REVISION):$(FF_AGE)"

EXTRA_SCRIPTS = generate_codepoint_selector.py

# "make is_LIGATURE" or "make -C fontforge is_LIGATURE" to regenerate
# is_LIGATURE.[ch]. You need LibUnicodeNames with Python bindings, and
# you need cproto.
is_LIGATURE:
	$(PYTHON) $(srcdir)/generate_codepoint_selector.py \
		'LIGATURE|VULGAR FRACTION' \
		is_LIGATURE_or_VULGAR_FRACTION > $(srcdir)/is_LIGATURE.c
	$(CPROTO) $(srcdir)/is_LIGATURE.c > $(srcdir)/is_LIGATURE.h

#--------------------------------------------------------------------------

if GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = alignment.c anchorsaway.c				\
	autowidth2dlg.c basedlg.c bdfinfo.c bitmapdlg.c bitmapview.c	\
	charinfo.c charview.c clipui.c combinations.c contextchain.c	\
	cursors.c cvaddpoints.c cvdebug.c cvdgloss.c cvexportdlg.c		\
	cvfreehand.c cvgetinfo.c cvgridfit.c cvhand.c cvhints.c			\
	cvimportdlg.c cvknife.c cvpalettes.c cvpointer.c cvruler.c		\
	cvshapes.c cvstroke.c cvtranstools.c displayfonts.c effectsui.c	\
	encodingui.c fontinfo.c fontview.c freetypeui.c fvfontsdlg.c	\
	fvmetricsdlg.c gotodlg.c groupsdlg.c histograms.c images.c		\
	kernclass.c layer2layer.c lookupui.c macencui.c math.c			\
	metricsview.c mmdlg.c nonlineartransui.c oflib.c openfontdlg.c	\
	prefs.c problems.c pythonui.c savefontdlg.c scriptingdlg.c		\
	scstylesui.c searchview.c sftextfield.c showatt.c simplifydlg.c	\
	splashimage.c startui.c statemachine.c tilepath.c transform.c	\
	ttfinstrsui.c uiutil.c windowmenu.c justifydlg.c deltaui.c		\
	usermenu.c

else !GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = startnoui.c

endif !GRAPHICAL_USER_INTERFACE

nodist_libfontforgeexe_la_SOURCES = exelibstamp.c

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
