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

if MAINTAINER_TOOLS
noinst_PROGRAMS = toengb
toengb_SOURCES = toengb.c
endif MAINTAINER_TOOLS

PO_FILES = ca.po el.po es.po it.po pl.po vi.po zh_TW.po de.po	\
	en_GB.po fr.po ja.po ml.po ru.po uk.po zh_CN.po pt.po

MO_FILES = ca.mo el.mo es.mo it.mo pl.mo vi.mo zh_TW.mo de.mo	\
	en_GB.mo fr.mo ja.mo ml.mo ru.mo uk.mo zh_CN.mo pt.mo

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

# Update Project-Id-Version string for each PO file with the current PACKAGE VERSION values.
# Based on example at https://answers.launchpad.net/launchpad/+question/121821 given
# by Jeroen T. Vermeulen (jtv) on 2010-08-20. Modified to use in Fontforge by JDS-2012sep27
update-project-id:
	(for localefile in $(PO_FILES); do \
		$(SED) -e "s/[Pp]roject-[Ii]d-[Vv]ersion:*[^\\]*/Project-Id-Version: $(PACKAGE) $(VERSION)/" "$${localefile}" >"$${localefile}.new"; \
		mv -b -- "$${localefile}.new" "$${localefile}"; \
	done)

EXTRA_DIST = $(PO_FILES)
MOSTLYCLEANFILES = $(MO_FILES)

-include $(top_srcdir)/git.mk
