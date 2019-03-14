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

PACKAGEROOTDIR = ${datarootdir}

# Where .desktop files go.
desktopdir = $(PACKAGEROOTDIR)/applications
desktop_DATA = org.fontforge.FontForge.desktop

appdatadir = $(PACKAGEROOTDIR)/appdata
appdata_DATA = org.fontforge.FontForge.appdata.xml

# Where metainfo xml files go
metainfodir = $(PACKAGEROOTDIR)/metainfo
metainfo_DATA = org.fontforge.FontForge.metainfo.xml org.fontforge.FontForge.appdata.xml

# Where MIME data goes. (Support for .sfd files, for instance.)
mimedir = $(PACKAGEROOTDIR)/mime
mimepackagesdir = $(PACKAGEROOTDIR)/mime/packages
mimepackages_DATA = fontforge.xml

# Where desktop icons go.
if USE_THEME_TANGO
ICON_SRC = $(srcdir)/tango
endif

if USE_THEME_2012
ICON_SRC = $(srcdir)/2012
endif

ICONSDIR = $(PACKAGEROOTDIR)/icons
HICOLORDIR = $(ICONSDIR)/hicolor
hicolor16x16dir = $(HICOLORDIR)/16x16/apps
hicolor22x22dir = $(HICOLORDIR)/22x22/apps
hicolor24x24dir = $(HICOLORDIR)/24x24/apps
hicolor32x32dir = $(HICOLORDIR)/32x32/apps
hicolor48x48dir = $(HICOLORDIR)/48x48/apps
hicolor64x64dir = $(HICOLORDIR)/64x64/apps
hicolor128x128dir = $(HICOLORDIR)/128x128/apps
hicolor256x256dir = $(HICOLORDIR)/256x256/apps
hicolor512x512dir = $(HICOLORDIR)/512x512/apps
hicolorscalabledir = $(HICOLORDIR)/scalable/apps

hicolor16x16_DATA = $(ICON_SRC)/16x16/org.fontforge.FontForge.png
hicolor22x22_DATA = $(ICON_SRC)/22x22/org.fontforge.FontForge.png
hicolor24x24_DATA = $(ICON_SRC)/24x24/org.fontforge.FontForge.png
hicolor32x32_DATA = $(ICON_SRC)/32x32/org.fontforge.FontForge.png
hicolor48x48_DATA = $(ICON_SRC)/48x48/org.fontforge.FontForge.png
hicolor64x64_DATA = $(ICON_SRC)/64x64/org.fontforge.FontForge.png
hicolor128x128_DATA = $(ICON_SRC)/128x128/org.fontforge.FontForge.png
hicolor256x256_DATA = $(ICON_SRC)/256x256/org.fontforge.FontForge.png
hicolor512x512_DATA = $(ICON_SRC)/512x512/org.fontforge.FontForge.png
hicolorscalable_DATA = $(ICON_SRC)/scalable/org.fontforge.FontForge.svg

deskpixsdir = $(PACKAGEROOTDIR)/pixmaps
deskpixs_DATA = $(ICON_SRC)/pixmaps/org.fontforge.FontForge.xpm	\
		$(ICON_SRC)/32x32/org.fontforge.FontForge.png

EXTRA_DIST = \
	2012/src/icon-16x16-apps-fontforge.svg		\
	2012/src/icon-24x24-apps-fontforge.svg		\
	2012/src/icon-32x32-apps-fontforge.svg		\
	2012/16x16/org.fontforge.FontForge.png		\
	2012/22x22/org.fontforge.FontForge.png		\
	2012/24x24/org.fontforge.FontForge.png		\
	2012/32x32/org.fontforge.FontForge.png		\
	2012/48x48/org.fontforge.FontForge.png		\
	2012/64x64/org.fontforge.FontForge.png		\
	2012/128x128/org.fontforge.FontForge.png	\
	2012/256x256/org.fontforge.FontForge.png	\
	2012/512x512/org.fontforge.FontForge.png	\
	2012/pixmaps/org.fontforge.FontForge.xpm	\
	2012/scalable/org.fontforge.FontForge.svg	\
	tango/src/icon-16x16-apps-fontforge.svg		\
	tango/src/icon-24x24-apps-fontforge.svg		\
	tango/src/icon-32x32-apps-fontforge.svg		\
	tango/16x16/org.fontforge.FontForge.png		\
	tango/22x22/org.fontforge.FontForge.png		\
	tango/24x24/org.fontforge.FontForge.png		\
	tango/32x32/org.fontforge.FontForge.png		\
	tango/48x48/org.fontforge.FontForge.png		\
	tango/64x64/org.fontforge.FontForge.png		\
	tango/128x128/org.fontforge.FontForge.png	\
	tango/256x256/org.fontforge.FontForge.png	\
	tango/512x512/org.fontforge.FontForge.png	\
	tango/pixmaps/org.fontforge.FontForge.xpm	\
	tango/scalable/org.fontforge.FontForge.svg	\
	icons-older/ffanvil32.gif			\
	icons-older/FFbutton-32.png			\
	icons-older/FFslug-32.png			\
	icons-older/fontforge.png			\
	icons-older/fontforge.xpm			\
	icons-older/FFbutton-128.png			\
	icons-older/FFslug-128.png			\
	icons-older/FFslug-48.png			\
	icons-older/fontforge.svg			\
	org.fontforge.FontForge.appdata.xml		\
	org.fontforge.FontForge.desktop			\
	fontforge.xml					\
	org.fontforge.FontForge.metainfo.xml

-include $(top_srcdir)/git.mk
