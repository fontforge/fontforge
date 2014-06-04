# Copyright (C) 2013 by Ben Martin
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

SUBDIRS = Inputbox.nib  MainMenu.nib  Msgbox.nib  PopUpButton.nib	\
	Progressbar.nib  SecureInputbox.nib  Textbox.nib

FFAPP_FILES = atom.icns cocoadialog.icns computer.icns document.icns	\
	find.icns finder.icns firewire.icns folder.icns gear.icns	\
	globe.icns hazard.icns heart.icns hourglass.icns info.icns	\
	ipod.icns person.icns sound.icns x.icns

EXTRA_DIST = $(FFAPP_FILES) Changelog $(srcdir)/*.icns $(srcdir)/*.plist $(srcdir)/*.strings

if PLATFORM_OSX
   ffappdir = $(pkgdatadir)/osx/FontForge.app/Contents/MacOS/CocoaDialog.app/Contents/Resources
   ffapp_DATA = $(FFAPP_FILES)
endif

-include $(top_srcdir)/git.mk
