# This file is included by automake for "normal" builds, and by
# GNUmakefile in GNUstep builds.

FF_VERSION = 1
FF_REVISION = 0
FF_AGE = 0

LIBFF_C_SRCFILES = asmfpst.c autohint.c autosave.c autotrace.c			\
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

NODIST_LIBFF_C_SRCFILES = libstamp.c

EXTRA_LIBFF_C_SRCFILES = splinerefigure.c

LIBFFEXE_C_SRCFILES = alignment.c anchorsaway.c autowidth2dlg.c			\
	basedlg.c bdfinfo.c bitmapdlg.c bitmapview.c charinfo.c				\
	charview.c clipui.c combinations.c contextchain.c cursors.c			\
	cvaddpoints.c cvdebug.c cvdgloss.c cvexportdlg.c cvfreehand.c		\
	cvgetinfo.c cvgridfit.c cvhand.c cvhints.c cvimportdlg.c			\
	cvknife.c cvpalettes.c cvpointer.c cvruler.c cvshapes.c				\
	cvstroke.c cvtranstools.c displayfonts.c effectsui.c encodingui.c	\
	fontinfo.c fontview.c freetypeui.c fvfontsdlg.c fvmetricsdlg.c		\
	gotodlg.c groupsdlg.c histograms.c images.c kernclass.c				\
	layer2layer.c lookupui.c macencui.c math.c metricsview.c mmdlg.c	\
	nonlineartransui.c oflib.c openfontdlg.c prefs.c problems.c			\
	pythonui.c savefontdlg.c scriptingdlg.c scstylesui.c searchview.c	\
	sftextfield.c showatt.c simplifydlg.c splashimage.c startui.c		\
	statemachine.c tilepath.c transform.c ttfinstrsui.c uiutil.c		\
	windowmenu.c justifydlg.c deltaui.c usermenu.c gnustepresources.c

NODIST_LIBFFEXE_C_SRCFILES = exelibstamp.c

FF_C_SRCFILES = main.c

FF_H_SRCFILES = fontforgeui.h sftextfieldP.h configure-fontforge.h	\
	views.h

INST_FF_H_SRCFILES = autowidth2.h configure-fontforge.h fontforge.h		\
	libffstamp.h psfont.h stemdb.h autowidth.h delta.h fontforgevw.h	\
	lookups.h savefont.h ttf.h baseviews.h fvmetrics.h mm.h				\
	scriptfuncs.h ttfinstrs.h edgelist2.h namehash.h scripting.h		\
	uiinterface.h bezctx_ff.h edgelist.h groups.h nonlineartrans.h		\
	sd.h unicoderange.h bitmapcontrol.h encoding.h ofl.h search.h		\
	usermenu.h fffreetype.h PfEd.h sfd1.h ffpython.h import.h			\
	plugins.h sflayoutP.h print.h splinefont.h

FF_M_SRCFILES = gnustepappmain.m
