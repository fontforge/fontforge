MACKEYS_PO_FILES = en.po
MACKEYS_MO_FILES = en.mo

.po.mo:
	$(MSGFMT) -o $@ $<

all-local: $(MACKEYS_MO_FILES)

install-data-local:
	(for localefile in $(MACKEYS_MO_FILES); do \
		locale=`echo "$${localefile}" | $(SED) -e 's/\.mo$$//'`; \
		$(MKDIR_P) "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES"; \
		$(INSTALL_DATA) "$${localefile}" "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES/Mac-FontForge-MenuShortCuts.mo"; \
	done)

uninstall-local:
	(for localefile in $(MACKEYS_MO_FILES); do \
		locale=`echo "$${localefile}" | $(SED) -e 's/\.mo$$//'`; \
		$(MKDIR_P) "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES"; \
		rm -f "$(DESTDIR)$(localedir)/$${locale}/LC_MESSAGES/Mac-FontForge-MenuShortCuts.mo"; \
	done)

EXTRA_DIST = $(MACKEYS_PO_FILES)
MOSTLYCLEANFILES = $(MACKEYS_MO_FILES)
