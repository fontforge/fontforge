if MACINTOSH
SUBDIRS = mackeys
endif MACINTOSH

if MAINTAINER_TOOLS
noinst_PROGRAMS = toengb
toengb_SOURCES = toengb.c
endif MAINTAINER_TOOLS

include common.mk

.po.mo:
	$(MSGFMT) -o $@ $<

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

EXTRA_DIST = $(PO_FILES)
MOSTLYCLEANFILES = $(MO_FILES)
