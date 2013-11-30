# Copyright (C) 2000-2012 by George Williams
# Copyright (C) 2012 by Barry Schwartz
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# The name of the author may not be used to endorse or promote products
# derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

include $(top_srcdir)/mk/layout.am

#--------------------------------------------------------------------------

lib_LTLIBRARIES = libfontforge.la

#--------------------------------------------------------------------------

if MACINTOSH
macobjective.o: macobjective.m
	clang -g -arch x86_64 -arch i386 -c "${srcdir}/macobjective.m" -o macobjective.o
endif MACINTOSH

#--------------------------------------------------------------------------

SUBDIRS = pixmaps
if LIBZMQ
SUBDIRS += collab
endif LIBZMQ

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
	$(SED) -e '1,$$ s/REPLACE_ME_WITH_MAJOR_VERSION/$(VERSION_MAJOR)/'	\
	       -e '1,$$ s/REPLACE_ME_WITH_MINOR_VERSION/$(VERSION_MINOR)/'	\
	    < $< > $@

#--------------------------------------------------------------------------

noinst_HEADERS = fontforgeui.h sftextfieldP.h configure-fontforge.h	\
	views.h is_LIGATURE.h flaglist.h strlist.h charview_private.h
pkginclude_HEADERS = autowidth2.h configure-fontforge.h fontforge.h		\
	libffstamp.h psfont.h stemdb.h autowidth.h delta.h fontforgevw.h	\
	lookups.h savefont.h ttf.h baseviews.h fvmetrics.h mm.h			\
	scriptfuncs.h ttfinstrs.h edgelist2.h namehash.h scripting.h		\
	uiinterface.h bezctx_ff.h edgelist.h groups.h nonlineartrans.h		\
	sd.h unicoderange.h bitmapcontrol.h encoding.h ofl.h search.h		\
	usermenu.h fffreetype.h PfEd.h sfd1.h ffpython.h			\
	plugins.h sflayoutP.h print.h splinefont.h cvruler.h ffglib.h

#--------------------------------------------------------------------------

LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la			\
	$(top_builddir)/gutils/libgioftp.la


AM_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"		\
	"-DSHAREDIR=\"${MY_SHAREDIR}\"" "-DDOCDIR=\"${MY_DOCDIR}\""	\
	"-DPLUGINDIR=\"${MY_PLUGINDIR}\"" $(MY_CFLAGS)

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
	activeinui.c pluginloading.c is_LIGATURE.c flaglist.c strlist.c \
        collabclient.c collabclient.h collabclientpriv.h

nodist_libfontforge_la_SOURCES = libstamp.c
EXTRA_libfontforge_la_SOURCES = splinerefigure.c

libfontforge_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBZMQ_CFLAGS)
libfontforge_la_LIBADD = splinerefigure.lo $(LIBADD) $(LIBLTDL)

if PLUGIN_GB12345
libfontforge_la_LIBADD += -dlopen $(top_builddir)/plugins/gb12345.la
endif PLUGIN_GB12345

if LIBZMQ
libfontforge_la_LIBADD += $(top_builddir)/collab/libzmqcollab.la $(LIBZMQ_LIBS)
endif LIBZMQ

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libfontforge_la_LDFLAGS = $(MY_CFLAGS) -release $(VERSION)
libfontforge_la_LIBADD += $(MY_LIBS) $(LIBZMQ_LIBS)

if GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = alignment.c anchorsaway.c   \
	autowidth2dlg.c basedlg.c bdfinfo.c bitmapdlg.c bitmapview.c	\
	charinfo.c charview.c clipui.c combinations.c contextchain.c	\
	cursors.c cvaddpoints.c cvdebug.c cvdgloss.c cvexportdlg.c		\
	cvfreehand.c cvgetinfo.c cvgridfit.c cvhand.c cvhints.c			\
	cvimportdlg.c cvknife.c cvpalettes.c cvpointer.c cvruler.c 		\
	cvshapes.c cvstroke.c cvtranstools.c displayfonts.c effectsui.c	\
	encodingui.c fontinfo.c fontview.c freetypeui.c fvfontsdlg.c	\
	fvmetricsdlg.c gotodlg.c groupsdlg.c histograms.c images.c		\
	kernclass.c layer2layer.c lookupui.c macencui.c math.c			\
	metricsview.c mmdlg.c nonlineartransui.c oflib.c openfontdlg.c	        \
	prefs.c problems.c pythonui.c savefontdlg.c scriptingdlg.c		\
	scstylesui.c searchview.c sftextfield.c showatt.c simplifydlg.c	        \
	splashimage.c startui.c statemachine.c tilepath.c transform.c	        \
	ttfinstrsui.c uiutil.c windowmenu.c justifydlg.c deltaui.c		\
	usermenu.c collabclientui.c collabclientui.h                    \
	sfundo.c sfundo.h wordlistparser.c wordlistparser.h

else !GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = startnoui.c

endif !GRAPHICAL_USER_INTERFACE

nodist_libfontforgeexe_la_SOURCES = exelibstamp.c

if MACINTOSH
libfontforgeexe_la_SOURCES += macobjective.h
MACFFEXE_LIBADD=macobjective.o
endif MACINTOSH

libfontforgeexe_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBZMQ_CFLAGS)
libfontforgeexe_la_LIBADD = libfontforge.la $(LTDLDEPS) $(LIBADD) $(GUILIBADD) $(MACFFEXE_LIBADD)

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libfontforgeexe_la_LDFLAGS = $(MY_CFLAGS) -release $(VERSION)
libfontforgeexe_la_LIBADD += $(MY_LIBS) $(LIBZMQ_LIBS) 
if LIBZMQ
libfontforgeexe_la_LIBADD += $(top_srcdir)/collab/libzmqcollab.la
endif

#--------------------------------------------------------------------------

LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la			\
	$(top_builddir)/gutils/libgioftp.la

GUILIBADD =
if GRAPHICAL_USER_INTERFACE
GUILIBADD += $(top_builddir)/gdraw/libgdraw.la $(XINPUT_LIBS)	\
	$(XKB_LIBS)
endif GRAPHICAL_USER_INTERFACE

LDADD = $(LIBADD) $(GUILIBADD)

AM_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"		\
	"-DSHAREDIR=\"${MY_SHAREDIR}\"" "-DDOCDIR=\"${MY_DOCDIR}\""	\
	"-DPLUGINDIR=\"${MY_PLUGINDIR}\"" $(MY_CFLAGS)

#--------------------------------------------------------------------------

# In splinerefigure.c, the slight error introduced occasionally by
# optimization turns out to have disastrous effects. Compile it
# without optimization.
splinerefigure.lo: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(LIBTOOL) --mode=compile $(CC) -g -c -o		\
		splinerefigure.lo $(AM_CPPFLAGS) $(srcdir)/splinerefigure.c

#--------------------------------------------------------------------------

EXTRA_DIST = fontimage.pe fontlint.pe sfddiff.pe libstamp.pre	\
	exelibstamp.pre fontimage.1 fontlint.1 sfddiff.1

if MACINTOSH
EXTRA_DIST += MacFontForgeApp.zip darwinsetup.in macobjective.m
endif MACINTOSH

MOSTLYCLEANFILES = fontimage fontlint sfddiff libstamp.c	\
	exelibstamp.c
EXTRA_DIST = libstamp.pre
MOSTLYCLEANFILES = libstamp.c

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------

FontForge-MenuShortCuts.pot: *.c
	$(XGETTEXT) -kH_ -oFontForge-MenuShortCuts.pot *.c ../fontforgeexe/*c ../gdraw/*.c ../gutils/*.c ../inc/*.h
	-patch < potmstitle.patch

FontForge.pot: *.c utf8.pot
	echo "If this doesn't work try make FontForge-old.pot instead"
	$(XGETTEXT) --from-code=utf-8 -k_ -kN_ -kS_ -kU_ -kNU_ -kP_:1,2 --add-comments=GT: -oFontForge.pot *.c ../fontforgeexe/*c ../gdraw/*.c ../gutils/*.c ../inc/*.h
	-patch < pottitle.patch

# xgettext has caniptions and strips any non-ASCII characters from my strings
# but I do occasionally want to use unicode characters (TeX, arrows, Increment)
# so I have put these messages in utf8.pot which I manage by hand. non-ASCII
# strings work just fine as keys. It does mean that my pot files should be
# viewed in utf8 though.

# I am told that new versions of xgettext support an option --from-code=UTF-8
# unfortunately this works on none of the systems I use, and I haven't found
# it documented. If it worked I would not need utf8.pot.
FontForge-old.pot: *.c utf8.pot
	$(XGETTEXT) -k_ -kN_ -kS_ -kP_:1,2 --add-comments=GT: -oFontForge.pot *.c ../fontforgeexe/*c ../gdraw/*.c ../gutils/*.c ../inc/*.h
	cat utf8.pot >> FontForge.pot
	-patch < pottitle.patch

#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
