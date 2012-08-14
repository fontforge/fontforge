include doc_list.mk

# Targets for maintainers.
#update: update-doc_list.mk
#update-doc_list.mk: $(srcdir)/doc_list.mk
#$(srcdir)/doc_list.mk: $(srcdir)/RUN_ME_TO_CHANGE_THE_DOCS.sh
#	(cd $(srcdir); /bin/sh RUN_ME_TO_CHANGE_THE_DOCS.sh)

nobase_doc_DATA = $(INSTALLED_DOC_LIST) .htaccess

EXTRA_DIST = $(DOC_LIST) .htaccess RUN_ME_TO_CHANGE_THE_DOCS.sh	\
	$(srcdir)/doc_list.mk

#MAINTAINERCLEANFILES = $(srcdir)/doc_list.mk
