if MACINTOSH
SUBDIRS = mackeys
endif MACINTOSH

if MAINTAINER_TOOLS
noinst_PROGRAMS = toengb
toengb_SOURCES = toengb.c
endif MAINTAINER_TOOLS

PO_FILES = ca.po el.po es.po it.po pl.po vi.po zh_TW.po de.po	\
	en_GB.po fr.po ja.po ml.po ru.po uk.po zh_CN.po

MO_FILES = ca.mo el.mo es.mo it.mo pl.mo vi.mo zh_TW.mo de.mo	\
	en_GB.mo fr.mo ja.mo ml.mo ru.mo uk.mo zh_CN.mo

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
