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

if THE_PROGRAMS

bin_PROGRAMS = fontforge
dist_man_MANS = fontforge.1

if NATIVE_SCRIPTING
nodist_bin_SCRIPTS = fontimage fontlint sfddiff
dist_man_MANS += fontimage.1 fontlint.1 sfddiff.1
endif NATIVE_SCRIPTING

endif THE_PROGRAMS

lib_LTLIBRARIES = libfontforgeexe.la

SUBDIRS = pixmaps
if LIBZMQ
SUBDIRS += collab
endif LIBZMQ

ACORNOBJS = acorn2sfd.o

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

acorn2sfd: $(ACORNOBJS)
	$(LIBTOOL) --mode=link $(CC) -o acorn2sfd $(ACORNOBJS) libfontforgeexe.la $(LDADD)

#--------------------------------------------------------------------------

LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la	\
	$(top_builddir)/gutils/libgioftp.la	\
	$(top_builddir)/fontforge/libfontforge.la \
	$(top_builddir)/lib/libgnu.la

GUILIBADD =
if GRAPHICAL_USER_INTERFACE
GUILIBADD += $(top_builddir)/gdraw/libgdraw.la $(XINPUT_LIBS)	\
	$(XKB_LIBS)
endif GRAPHICAL_USER_INTERFACE

LDADD = $(LIBADD) $(GUILIBADD)

AM_CFLAGS = $(WARN_CFLAGS)

AM_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	"-I$(top_builddir)/lib" "-I$(top_srcdir)/lib"			\
              "-I$(top_srcdir)/fontforge" \
	"-DSHAREDIR=\"${MY_SHAREDIR}\"" "-DDOCDIR=\"${MY_DOCDIR}\""	\
	"-DPLUGINDIR=\"${MY_PLUGINDIR}\"" $(MY_CFLAGS)
#--------------------------------------------------------------------------

if THE_PROGRAMS

fontforge_SOURCES = main.c
fontforge_CPPFLAGS = $(AM_CPPFLAGS)
fontforge_LDADD = libfontforgeexe.la $(LDADD)
fontforge_LDFLAGS = $(MY_CFLAGS) -export-dynamic
if LIBZMQ
fontforge_LDADD += $(top_builddir)/collab/libzmqcollab.la $(MY_LIBS) $(LIBZMQ_LIBS)
endif

endif THE_PROGRAMS

#--------------------------------------------------------------------------

libfontforgeexe_la_CPPFLAGS = $(AM_CPPFLAGS)

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
	metricsview.c mmdlg.c nonlineartransui.c openfontdlg.c	        \
	prefs.c problems.c pythonui.c savefontdlg.c scriptingdlg.c		\
	scstylesui.c searchview.c sftextfield.c showatt.c simplifydlg.c	        \
	splashimage.c startui.c statemachine.c tilepath.c transform.c	        \
	ttfinstrsui.c uiutil.c windowmenu.c justifydlg.c deltaui.c		\
	usermenu.c macobjective.h                                               \
	collabclientui.c collabclientui.h                                       \
	sfundo.c wordlistparser.c wordlistparser.h fontforgeexe.h

EXTRA_libfontforgeexe_la_SOURCES = macobjective.m

libfontforgeexe_la_CPPFLAGS +=  $(LIBZMQ_CFLAGS) $(GTK2_CFLAGS)

if MACINTOSH
libfontforgeexe_la_OBJCFLAGS = -I../osx/FontForge.app/Contents/Frameworks/Breakpad.framework/Headers
libfontforgeexe_la_SOURCES += macobjective.m
endif

else !GRAPHICAL_USER_INTERFACE

libfontforgeexe_la_SOURCES = startnoui.c

endif !GRAPHICAL_USER_INTERFACE


libfontforgeexe_la_LIBADD = $(LTDLDEPS) $(LIBADD) $(GUILIBADD) $(MY_LIBS) $(GTK2_LIBS) $(top_builddir)/fontforge/libfontforge.la $(BREAKPAD_LIBADD)

libfontforgeexe_la_LDFLAGS = $(MY_CFLAGS) -version-info $(LIBFONTFORGEEXE_VERSION) \
	${MY_LIB_LDFLAGS}

if LIBZMQ
libfontforgeexe_la_LIBADD += $(top_builddir)/collab/libzmqcollab.la $(LIBZMQ_LIBS)
endif

if PYTHON_SCRIPTING
libfontforgeexe_la_LIBADD += $(PYTHON_LIBS)
endif PYTHON_SCRIPTING

#--------------------------------------------------------------------------

EXTRA_DIST = fontimage.pe fontlint.pe sfddiff.pe	\
	fontimage.1 fontlint.1 sfddiff.1		\
	acorn2sfd.c darwinsetup.in
# MacFontForgeApp.zip
MOSTLYCLEANFILES = fontimage fontlint sfddiff
DISTCLEANFILES = $(MOSTLYCLEANFILES)

#--------------------------------------------------------------------------
#--------------------------------------------------------------------------
#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
