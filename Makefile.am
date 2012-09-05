ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS =
AM_LDFLAGS =

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =
MOSTLYCLEANFILES =

include_HEADERS =
noinst_LTLIBRARIES =
lib_LTLIBRARIES =
EXTRA_LTLIBRARIES =

include libltdl/Makefile.inc

SUBDIRS = inc
SUBDIRS += Unicode gutils

if GRAPHICAL_USER_INTERFACE
SUBDIRS += gdraw
endif GRAPHICAL_USER_INTERFACE

SUBDIRS += plugins
SUBDIRS += fontforge po htdocs pycontrib
SUBDIRS += cidmap
SUBDIRS += $(subdirs)
SUBDIRS += tests

pkgconfigdir = ${libdir}/pkgconfig
nodist_pkgconfig_DATA = libfontforge.pc libfontforgeexe.pc

EXTRA_DIST += LICENSE README-Mac.html README-MS.html README-unix	\
	README-Unix.html Readme.VMS





#install_incs:
#	mkdir -p $(DESTDIR)$(includedir)/fontforge
#	$(INSTALL_DATA) $(srcdir)/inc/*.h $(DESTDIR)$(includedir)/fontforge
##	$(INSTALL_DATA) `grep -L GGadget $(srcdir)/fontforge/*.h` $(DESTDIR)$(includedir)/fontforge
#	( for file in $(srcdir)/fontforge/*.h ; do if grep GGadget $$file >/dev/null 2>&1 ; then echo pointless >/dev/null ; else $(INSTALL_DATA) $$file $(DESTDIR)$(includedir)/fontforge ; fi ; done )


##explicit use of sh to make sure we don't invoke csh with different "if" syntax
#	sh -c "if test -d $(srcdir)/htdocs >/dev/null ; then $(MAKE) install_docs ; fi"
#	sh -c "if test -d fontforge/FontForge.app >/dev/null ; then rm -rf /Applications/FontForge.app ; mv fontforge/FontForge.app /Applications ; ln -s /usr/local/bin/fontforge /Applications/FontForge.app/Contents/MacOS/FontForge ; fi"
#	sh < systemspecific


#install_py: $(DESTDIR)$(sharedir) all install_libs install_incs install_po install_prog install_pycontrib
## You may need to be root to do the install
##explicit use of sh to make sure we don't invoke csh with different "if" syntax
#	sh -c "if test -d $(srcdir)/htdocs >/dev/null ; then $(MAKE) install_docs ; for dir in * ; do if test -d $$dir ; then  rm -rf $$dir ; fi; done ; fi"
#	sh -c "if test -d fontforge/FontForge.app >/dev/null ; then rm -rf /Applications/FontForge.app ; mv fontforge/FontForge.app /Applications ; ln -s /usr/local/bin/fontforge /Applications/FontForge.app/Contents/MacOS/FontForge ; fi"
#	sh < systemspecific
#	-(cd pyhook ; python setup.py bdist --formats=gztar ; cp dist/fontforge-*.tar.gz $(DESTDIR)/tmp; for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done )


#uninstall:
#	-rm $(DESTDIR)$(sharedir)/pfaedit-*.ui
