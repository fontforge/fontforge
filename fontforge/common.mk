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

# FIXME: Decide which of these to install.
FF_H_SRCFILES = autowidth2.h ffpython.h nonlineartrans.h sfd1.h	\
	autowidth.h fontforge.h ofl.h sflayoutP.h baseviews.h			\
	fontforgeui.h PfEd.h sftextfieldP.h bezctx_ff.h fontforgevw.h	\
	plugins.h splinefont.h bitmapcontrol.h fvmetrics.h print.h		\
	stemdb.h configure-fontforge.h groups.h psfont.h ttf.h delta.h	\
	import.h savefont.h ttfinstrs.h edgelist2.h libffstamp.h		\
	scriptfuncs.h uiinterface.h edgelist.h lookups.h scripting.h	\
	unicoderange.h encoding.h mm.h sd.h views.h fffreetype.h		\
	namehash.h search.h usermenu.h

INST_FF_H_SRCFILES =

FF_M_SRCFILES = gnustepappmain.m
