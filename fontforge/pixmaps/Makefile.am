include png_list.mk

pixmapsdir = "${pkgdatadir}/pixmaps"

dist_pixmaps_DATA = $(PNG_LIST)

# Targets for maintainers.
update: update-png_list.mk
update-png_list.mk: $(srcdir)/png_list.mk
$(srcdir)/png_list.mk: $(srcdir)/RUN_ME_TO_CHANGE_THE_PNGS.sh
	(cd $(srcdir); /bin/sh RUN_ME_TO_CHANGE_THE_PNGS.sh)

EXTRA_DIST = RUN_ME_TO_CHANGE_THE_PNGS.sh $(srcdir)/png_list.mk

MOSTLYCLEANFILES = GNUmakefile GNUmakefile.preamble	\
	GNUmakefile.postamble

MAINTAINERCLEANFILES = $(srcdir)/png_list.mk
