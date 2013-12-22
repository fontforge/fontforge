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

#SUBDIRS = pixmaps
#if LIBZMQ
#SUBDIRS = collab
#endif LIBZMQ

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

libfontforge_la_SOURCES = activeinui.c asmfpst.c autohint.c autosave.c	\
	autotrace.c autowidth.c autowidth2.c bezctx_ff.c bitmapchar.c	\
	bitmapcontrol.c bvedit.c collabclient.c collabclient.h		\
	collabclientpriv.h clipnoui.c crctab.c cvexport.c cvimages.c	\
	cvundoes.c dumpbdf.c dumppfa.c effects.c encoding.c		\
	featurefile.c flaglist.c fontviewbase.c freetype.c		\
	fvcomposite.c fvfonts.c fvimportbdf.c fvmetrics.c ftdelta.c	\
	glyphcomp.c groups.c http.c ikarus.c is_LIGATURE.c langfreq.c	\
	lookups.c macbinary.c macenc.c mathconstants.c mm.c namelist.c	\
	nonlineartrans.c noprefs.c nouiutil.c nowakowskittfinstr.c	\
	ofl.c othersubrs.c palmfonts.c parsepdf.c parsepfa.c		\
	parsettfatt.c parsettfbmf.c parsettf.c parsettfvar.c plugins.c	\
	pluginloading.c print.c psread.c pua.c python.c savefont.c	\
	scripting.c scstyles.c search.c sfd1.c sfd.c sflayout.c		\
	sfundo.h spiro.c splinechar.c splinefill.c splinefont.c		\
	splineorder2.c splineoverlap.c splinesaveafm.c splinesave.c	\
	splinestroke.c splineutil2.c splineutil.c stamp.c start.c	\
	stemdb.c strlist.c svg.c tottfaat.c tottfgpos.c tottf.c		\
	tottfvar.c ttfinstrs.c ttfspecial.c ufo.c unicoderange.c	\
	utils.c winfonts.c woff.c zapfnomen.c libstamp.c

EXTRA_libfontforge_la_SOURCES = splinerefigure.c

libfontforge_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBZMQ_CFLAGS)
libfontforge_la_LIBADD = splinerefigure.lo $(LIBADD) $(LIBLTDL)

if PLUGIN_GB12345
libfontforge_la_LIBADD += -dlopen $(top_builddir)/plugins/gb12345.la
endif PLUGIN_GB12345

if LIBZMQ
libfontforge_la_LIBADD += $(top_builddir)/collab/libzmqcollab.la $(LIBZMQ_LIBS)
endif LIBZMQ

libfontforge_la_LDFLAGS = $(MY_CFLAGS) -version-info $(LIBFONTFORGE_VERSION)
libfontforge_la_LIBADD += $(MY_LIBS) $(LIBZMQ_LIBS)

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

EXTRA_DIST = fontimage.pe fontlint.pe sfddiff.pe fontimage.1 fontlint.1	\
	sfddiff.1

if MACINTOSH
EXTRA_DIST += MacFontForgeApp.zip darwinsetup.in macobjective.m
endif MACINTOSH

MOSTLYCLEANFILES = fontimage fontlint sfddiff

#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
