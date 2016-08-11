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

noinst_FILES = FontForge.pot FontForge-MenuShortCuts.pot FontForge-MenuShortCuts.pot.orig FontForge-old.pot
nodist_FILES = $(noinst_FILES)

if MAINTAINER_TOOLS
noinst_PROGRAMS = toengb
toengb_SOURCES = toengb.c
endif MAINTAINER_TOOLS

PO_FILES = ca.po el.po es.po it.po pl.po vi.po zh_TW.po de.po	\
	en_GB.po fr.po ja.po ml.po ru.po uk.po zh_CN.po pt.po ko.po

MO_FILES = ca.mo el.mo es.mo it.mo pl.mo vi.mo zh_TW.mo de.mo	\
	en_GB.mo fr.mo ja.mo ml.mo ru.mo uk.mo zh_CN.mo pt.mo ko.mo

.po.mo:
	$(MSGFMT) --check --output-file=$@ $<

all-local: $(MO_FILES)

install-data-local:
	(for localefile in $(MO_FILES); do \
		locale=`echo "$${localefile}" | $(SED) -e 's/\.mo$$//'`; \
		$(MKDIR_P) "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES"; \
		$(INSTALL_DATA) "$${localefile}" "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES/FontForge.mo"; \
	done)

uninstall-local:
	(for localefile in $(MO_FILES); do \
		locale=`echo "$${localefile}" | $(SED) -e 's/\.mo$$//'`; \
		$(MKDIR_P) "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES"; \
		rm -f "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES/FontForge.mo"; \
	done)

#--------------------------------------------------------------------------
# Update Project-Id-Version string for each PO file with the current PACKAGE VERSION values.
# Based on example at https://answers.launchpad.net/launchpad/+question/121821 given
# by Jeroen T. Vermeulen (jtv) on 2010-08-20. Modified to use in Fontforge by JDS-2012sep27
update-project-id:
	(for localefile in $(PO_FILES); do \
		$(SED) -e "s/[Pp]roject-[Ii]d-[Vv]ersion:*[^\\]*/Project-Id-Version: $(PACKAGE) $(VERSION)/" "$${localefile}" >"$${localefile}.new"; \
		mv -b -- "$${localefile}.new" "$${localefile}"; \
	done)

#--------------------------------------------------------------------------
# These 'make FontForge*' potfiles are created in $(builddir).
#
# If creating or updating a language po file, also go take a look at file
# desktop/fontforge.appdata.xml to see if it also needs an extra language
# definition added or updated.

XGETTEXT_SEARCH_DIRS = $(top_srcdir)/fontforge/*.c $(top_srcdir)/fontforgeexe/*.c $(top_srcdir)/gdraw/*.c $(top_srcdir)/gutils/*.c $(top_srcdir)/inc/*.h

FontForge.pot:
	echo "If this doesn't work try make FontForge-old.pot instead"
	$(XGETTEXT) --from-code=utf-8 -k_ -kN_ -kS_ -kU_ -kNU_ -kP_:1,2 --add-comments=GT: -oFontForge.pot $(XGETTEXT_SEARCH_DIRS)
	-patch < pottitle.patch

FontForge-MenuShortCuts.pot:
	$(XGETTEXT) -kH_ -oFontForge-MenuShortCuts.pot $(XGETTEXT_SEARCH_DIRS)
	-patch < potmstitle.patch

# To update a language po file, go to directory po and type:
# make FontForge-Update-po 1=fr
# In this example (above), 'fr' is chosen to update po file 'fr.po'.
FontForge-Update-po:
	$(MAKE) FontForge.pot; $(MAKE) FontForge-MenuShortCuts.pot;	\
	mv $1.po $1-old.po; msgmerge -N $1-old.po FontForge.pot > $1.po; \
	rm FontForge.pot

# This command (below) is for systems with older less capable xgettext.
# xgettext has caniptions and strips non-ASCII characters from my strings
# but I occasionally want to use unicode characters (TeX, arrows, Increment)
# so I have put these messages in utf8.pot which I manage by hand. non-ASCII
# strings work just fine as keys. It does mean that my pot files should be
# viewed in utf8 though.

# I am told that new versions of xgettext support option --from-code=utf-8
# unfortunately this works on none of the systems I use and I have not
# found it documented. If it worked I would not need utf8.pot.
FontForge-old.pot: utf8.pot
	$(XGETTEXT) -k_ -kN_ -kS_ -kP_:1,2 --add-comments=GT: -oFontForge.pot $(XGETTEXT_SEARCH_DIRS)
	cat utf8.pot >> FontForge.pot
	-patch < pottitle.patch

#--------------------------------------------------------------------------

EXTRA_DIST = $(PO_FILES) potmstitle.patch pottitle.patch utf8.pot

MOSTLYCLEANFILES = $(MO_FILES) $(noinst_FILES)	\
	FontForge.pot FontForge-MenuShortCuts.pot *-old.po

-include $(top_srcdir)/git.mk
