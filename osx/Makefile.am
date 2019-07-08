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

CONTENTS_DIR = FontForge.app/Contents
MACOS_DIR = $(CONTENTS_DIR)/MacOS
RESOURCES_DIR = $(CONTENTS_DIR)/Resources

CONTENTS_FILES = \
	$(CONTENTS_DIR)/PkgInfo

MACOS_FILES = \
  $(MACOS_DIR)/FontForge \
  $(MACOS_DIR)/debug-script \
  $(MACOS_DIR)/Attach-Debugger-to-running-FontForge

RESOURCES_FILES = \
  $(RESOURCES_DIR)/buttonFF.icns \
  $(RESOURCES_DIR)/fontforge_backup.icns \
  $(RESOURCES_DIR)/fontforge_dfont.icns \
  $(RESOURCES_DIR)/FontForge.icns \
  $(RESOURCES_DIR)/fontforge_otf.icns \
  $(RESOURCES_DIR)/fontforge_ps3.icns \
  $(RESOURCES_DIR)/fontforge_ps.icns \
  $(RESOURCES_DIR)/fontforge_sfd.icns \
  $(RESOURCES_DIR)/fontforge_ttf.icns

LPROJ_FILES = \
  $(RESOURCES_DIR)/English.lproj/InfoPlist.string

if PLATFORM_OSX
  ffappcontentsdir = $(pkgdatadir)/osx/FontForge.app/Contents
  ffappcontents_DATA = $(CONTENTS_FILES)
  nodist_ffappcontents_DATA = $(CONTENTS_DIR)/Info.plist

  ffappmacosdir = $(pkgdatadir)/osx/FontForge.app/Contents/MacOS
  ffappmacos_SCRIPTS = $(MACOS_FILES)

  ffappresourcesdir = $(pkgdatadir)/osx/FontForge.app/Contents/Resources
  ffappresources_DATA = $(RESOURCES_FILES)

  ffapplprojdir = $(pkgdatadir)/osx/FontForge.app/Contents/Resources/English.lproj
  ffapplproj_DATA = $(LPROJ_FILES)
endif

EXTRA_DIST = \
	Portfile \
	$(CONTENTS_DIR)/Info.plist.in \
	$(CONTENTS_FILES) \
	$(MACOS_FILES) \
	$(RESOURCES_FILES) \
	$(LPROJ_FILES)

-include $(top_srcdir)/git.mk
