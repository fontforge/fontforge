FF_VERSION = 1
FF_REVISION = 0
FF_AGE = 0

bin_PROGRAMS = fontforge
lib_LTLIBRARIES = libfontforge.la libfontforgeexe.la
dist_man_MANS = fontforge.1 fontimage.1 fontlint.1 sfddiff.1

if NATIVE_SCRIPTING
nodist_bin_SCRIPTS = fontimage fontlint sfddiff
endif NATIVE_SCRIPTING

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

# FIXME: Decide which of these to install.
noinst_HEADERS = autowidth2.h ffpython.h nonlineartrans.h sfd1.h	\
	autowidth.h fontforge.h ofl.h sflayoutP.h baseviews.h			\
	fontforgeui.h PfEd.h sftextfieldP.h bezctx_ff.h fontforgevw.h	\
	plugins.h splinefont.h bitmapcontrol.h fvmetrics.h print.h		\
	stemdb.h configure-fontforge.h groups.h psfont.h ttf.h delta.h	\
	import.h savefont.h ttfinstrs.h edgelist2.h libffstamp.h		\
	scriptfuncs.h uiinterface.h edgelist.h lookups.h scripting.h	\
	unicoderange.h encoding.h mm.h sd.h views.h fffreetype.h		\
	namehash.h search.h usermenu.h

#--------------------------------------------------------------------------

fontforge_SOURCES = main.c
fontforge_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)
fontforge_LDADD = libfontforgeexe.la
fontforge_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -export-dynamic

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
	activeinui.c

nodist_libfontforge_la_SOURCES = libstamp.c

EXTRA_libfontforge_la_SOURCES = splinerefigure.c

libfontforge_la_CPPFLAGS = "-I$(top_builddir)/inc"			\
	"-I$(top_srcdir)/inc" "-DSHAREDIR=\"${pkgdatadir}\""	\
	"-DDOCDIR=\"${docdir}\"" $(MY_CFLAGS)

libfontforge_la_LIBADD = splinerefigure.lo ../Unicode/libgunicode.la	\
	../gutils/libgutils.la ../gutils/libgioftp.la
if GRAPHICAL_USER_INTERFACE
libfontforge_la_LIBADD += ../gdraw/libgdraw.la $(XINPUT_LIBS)	\
	$(XKB_LIBS)
endif GRAPHICAL_USER_INTERFACE

libfontforge_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -version-info	\
	"$(FF_VERSION):$(FF_REVISION):$(FF_AGE)"

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
libfontforgeexe_la_LIBADD = libfontforge.la
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

EXTRA_DIST = fontimage.pe fontlint.pe sfddiff.pe libstamp.pre exelibstamp.pre
MOSTLYCLEANFILES = fontimage fontlint sfddiff libstamp.c exelibstamp.c
