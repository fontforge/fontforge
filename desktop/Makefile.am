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

dist_desktop_DATA = fontforge.desktop
dist_metainfo_DATA = fontforge.appdata.xml
dist_hicolor16x16_DATA = icons/16x16/apps/fontforge.png
dist_hicolor22x22_DATA = icons/22x22/apps/fontforge.png
dist_hicolor24x24_DATA = icons/24x24/apps/fontforge.png
dist_hicolor32x32_DATA = icons/32x32/apps/fontforge.png
dist_hicolor48x48_DATA = icons/48x48/apps/fontforge.png
dist_hicolor64x64_DATA = icons/64x64/apps/fontforge.png
dist_hicolor128x128_DATA = icons/128x128/apps/fontforge.png
dist_hicolor256x256_DATA = icons/256x256/apps/fontforge.png
dist_hicolorscalable_DATA = icons/scalable/apps/fontforge.svg
dist_mimepackages_DATA = fontforge.xml

if XDG_DESKTOP

install-data-hook: update-mime-database update-desktop-database
uninstall-hook: update-mime-database update-desktop-database

update-mime-database:
	if test -z "$(DESTDIR)"; then $(UPDATE_MIME_DATABASE) ${mimedir} || : ; fi

update-desktop-database:
	if test -z "$(DESTDIR)"; then $(UPDATE_DESKTOP_DATABASE) ${desktopdir} || : ; fi

endif XDG_DESKTOP

EXTRA_DIST = icons/src/icon-16x16-apps-fontforge.svg	\
	icons/src/icon-24x24-apps-fontforge.svg		\
	icons/src/icon-32x32-apps-fontforge.svg

-include $(top_srcdir)/git.mk
