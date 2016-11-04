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

noinst_HEADERS = \
	asmfpst.h \
	autohint.h \
	autosave.h \
	autotrace.h \
	autowidth2.h \
	autowidth.h \
	bitmapchar.h \
	bitmapcontrol.h \
	bvedit.h \
	charview_private.h \
	clipnoui.h \
	configure-fontforge.h \
	crctab.h \
	cvexport.h \
	cvimages.h \
	cvundoes.h \
	dumpbdf.h \
	dumppfa.h \
	flaglist.h \
	fontforgeui.h \
	sftextfieldP.h \
	strlist.h \
	tables.h \
	views.h

pkginclude_HEADERS = \
	autowidth2.h \
	autowidth.h \
	baseviews.h \
	bezctx_ff.h \
	bitmapcontrol.h \
	configure-fontforge.h \
	cvruler.h \
	delta.h \
	edgelist2.h \
	edgelist.h \
	encoding.h \
	fffreetype.h \
	ffglib.h \
	ffpython.h \
	fontforge.h \
	fontforgevw.h \
	fvmetrics.h \
	glif_name_hash.h \
	groups.h \
	libffstamp.h \
	lookups.h \
	mm.h \
	namehash.h \
	nonlineartrans.h \
	ofl.h \
	PfEd.h \
	plugins.h \
	print.h \
	psfont.h \
	savefont.h \
	scriptfuncs.h \
	scripting.h \
	sd.h \
	search.h \
	sfd1.h \
	sflayoutP.h \
	splinefont.h \
	stemdb.h \
	ttf.h \
	ttfinstrs.h \
	uiinterface.h \
	unicoderange.h \
	usermenu.h

#--------------------------------------------------------------------------

LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la	\
	$(top_builddir)/gutils/libgioftp.la	\
	$(top_builddir)/lib/libgnu.la

AM_CFLAGS = $(WARN_CFLAGS)

AM_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"		\
	"-I$(top_builddir)/lib" "-I$(top_srcdir)/lib"			\
	"-DSHAREDIR=\"${MY_SHAREDIR}\"" "-DDOCDIR=\"${MY_DOCDIR}\""	\
        "-I$(top_srcdir)/uthash/src" "-DFF_UTHASH_GLIF_NAMES=1"         \
	"-DPLUGINDIR=\"${MY_PLUGINDIR}\"" $(MY_CFLAGS)

#--------------------------------------------------------------------------

libfontforge_la_SOURCES = \
	activeinui.c \
	asmfpst.c \
	autohint.c \
	autosave.c \
	autotrace.c \
	autowidth2.c \
	autowidth.c \
	bezctx_ff.c \
	bitmapchar.c \
	bitmapcontrol.c \
	bvedit.c \
	clipnoui.c \
	collabclient.c \
	collabclient.h \
	collabclientpriv.h \
	crctab.c \
	cvexport.c \
	cvimages.c \
	cvundoes.c \
	dumpbdf.c \
	dumppfa.c \
	effects.c \
	encoding.c \
	featurefile.c \
	flaglist.c \
	fontviewbase.c \
	freetype.c \
	ftdelta.c \
	fvcomposite.c \
	fvfonts.c \
	fvimportbdf.c \
	fvmetrics.c \
	glif_name_hash.c \
	glyphcomp.c \
	groups.c \
	http.c \
	ikarus.c \
	langfreq.c \
	lookups.c \
	macbinary.c \
	macenc.c \
	mathconstants.c \
	mm.c \
	namelist.c \
	nonlineartrans.c \
	noprefs.c \
	nouiutil.c \
	nowakowskittfinstr.c \
	ofl.c \
	othersubrs.c \
	palmfonts.c \
	parsepdf.c \
	parsepfa.c \
	parsettfatt.c \
	parsettfbmf.c \
	parsettf.c \
	parsettfvar.c \
	pluginloading.c \
	plugins.c \
	print.c \
	psread.c \
	pua.c \
	python.c \
	savefont.c \
	scripting.c \
	scstyles.c \
	search.c \
	sfd1.c \
	sfd.c \
	sflayout.c \
	sfundo.h \
	spiro.c \
	splinechar.c \
	splinefill.c \
	splinefont.c \
	splineorder2.c \
	splineoverlap.c \
	splinesaveafm.c \
	splinesave.c \
	splinestroke.c \
	splineutil2.c \
	splineutil.c \
	start.c \
	stemdb.c \
	strlist.c \
	svg.c \
	tottfaat.c \
	tottf.c \
	tottfgpos.c \
	tottfvar.c \
	ttfinstrs.c \
	ttfspecial.c \
	ufo.c \
	unicoderange.c \
	utils.c \
	winfonts.c \
	woff.c \
	zapfnomen.c

EXTRA_libfontforge_la_SOURCES = splinerefigure.c

libfontforge_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBZMQ_CFLAGS)
libfontforge_la_LIBADD = splinerefigure.lo $(LIBADD) $(LIBLTDL)

if PLUGIN_GB12345
libfontforge_la_LIBADD += -dlopen $(top_builddir)/plugins/gb12345.la
endif PLUGIN_GB12345

if LIBZMQ
libfontforge_la_LIBADD += $(top_builddir)/collab/libzmqcollab.la $(LIBZMQ_LIBS)
endif LIBZMQ

libfontforge_la_LDFLAGS = $(MY_CFLAGS) -version-info $(LIBFONTFORGE_VERSION) \
	${MY_LIB_LDFLAGS}

if PYTHON_SCRIPTING
if !MACINTOSH
libfontforge_la_LIBADD += $(PYTHON_LIBS)
else
libfontforge_la_LDFLAGS += -undefined dynamic_lookup
endif MACINTOSH
endif PYTHON_SCRIPTING

libfontforge_la_LIBADD += $(MY_LIBS)

#--------------------------------------------------------------------------

GUILIBADD =
if GRAPHICAL_USER_INTERFACE
GUILIBADD += $(top_builddir)/gdraw/libgdraw.la $(XINPUT_LIBS) $(XKB_LIBS)
endif GRAPHICAL_USER_INTERFACE

LDADD = $(LIBADD) $(GUILIBADD)

#--------------------------------------------------------------------------

# In splinerefigure.c, the slight error introduced occasionally by
# optimization turns out to have disastrous effects. Compile it
# without optimization.
splinerefigure.lo: $(srcdir)/splinerefigure.c $(srcdir)/splinefont.h
	$(LIBTOOL) --mode=compile $(CC) $(RAW_COMPILE_PLATFORM_CFLAGS) -g -c -o \
		splinerefigure.lo $(AM_CPPFLAGS) $(srcdir)/splinerefigure.c

#--------------------------------------------------------------------------
EXTRA_DIST = generate_codepoint_selector.py threaddummy.c

if MACINTOSH
EXTRA_DIST += MacFontForgeApp.zip
MOSTLYCLEANFILES = MacFontForgeApp.zip
endif MACINTOSH

#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
