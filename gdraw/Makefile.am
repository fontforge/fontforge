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

lib_LTLIBRARIES = libgdraw.la

libgdraw_la_SOURCES = \
	choosericons.c \
	colorP.h \
	ctlvalues.c \
	drawboxborder.c \
	fontP.h \
	gaskdlg.c \
	gbuttons.c \
	gchardlg.c \
	gcolor.c \
	gcontainer.c \
	gdrawable.c \
	gdrawbuildchars.c \
	gdraw.c \
	gdrawerror.c \
	gdrawgimage.c \
	gdrawP.h \
	gdrawtxt.c \
	gdrawtxtinit.c \
	gfilechooser.c \
	gfiledlg.c \
	ggadgetP.h \
	ggadgets.c \
	ggroupbox.c \
	ghvbox.c \
	gimagebmpP.h \
	gimageclut.c \
	gimagecvt.c \
	gimagepsdraw.c \
	gimagewriteeps.c \
	gimagexdraw.c \
	gkeysym.c \
	glist.c \
	gmatrixedit.c \
	gmenu.c \
	gprogress.c \
	gpsdraw.c \
	gpsdrawP.h \
	gpstxtinit.c \
	gradio.c \
	gresedit.c \
	gresource.c \
	gresourceimage.c \
	gresourceP.h \
	gsavefiledlg.c \
	gscrollbar.c \
	gspacer.c \
	gtabset.c \
	gtextfield.c \
	gtextinfo.c \
	gwidgetP.h \
	gwidgets.c \
	gxcdraw.c \
	gxcdrawP.h \
	gxdraw.c \
	gxdrawP.h \
	hotkeys.c \
	hotkeys.h \
	xkeysyms_unicode.c

libgdraw_la_CFLAGS = $(WARN_CFLAGS)

libgdraw_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	"-I$(top_builddir)/lib" "-I$(top_srcdir)/lib"			\
	"-I$(top_srcdir)/fontforge" "-DSHAREDIR=\"${MY_SHAREDIR}\"" $(MY_CFLAGS)

libgdraw_la_LIBADD = $(MY_LIBS) $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la \
	$(top_builddir)/fontforge/libfontforge.la \
	$(top_builddir)/lib/libgnu.la

if GRAPHICAL_USER_INTERFACE
libgdraw_la_LIBADD += $(XINPUT_LIBS)
endif GRAPHICAL_USER_INTERFACE

libgdraw_la_LDFLAGS = $(MY_CFLAGS) -version-info $(LIBGDRAW_VERSION) \
	${MY_LIB_LDFLAGS}

EXTRA_DIST = gdrawwacomdriver.c genkeysym.c growcol.c

-include $(top_srcdir)/git.mk
