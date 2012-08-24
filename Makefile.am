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
SUBDIRS += pluginloading
SUBDIRS += plugins
SUBDIRS += Unicode gutils

if GRAPHICAL_USER_INTERFACE
SUBDIRS += gdraw
endif GRAPHICAL_USER_INTERFACE

SUBDIRS += fontforge po htdocs pycontrib
SUBDIRS += $(subdirs)
SUBDIRS += tests

pkgconfigdir = ${libdir}/pkgconfig
nodist_pkgconfig_DATA = fontforge.pc libfontforge.pc	\
	libfontforgeexe.pc

EXTRA_DIST += LICENSE README-Mac.html README-MS.html README-unix	\
	README-Unix.html Readme.VMS







#install_incs:
#	mkdir -p $(DESTDIR)$(includedir)/fontforge
#	$(INSTALL_DATA) $(srcdir)/inc/*.h $(DESTDIR)$(includedir)/fontforge
##	$(INSTALL_DATA) `grep -L GGadget $(srcdir)/fontforge/*.h` $(DESTDIR)$(includedir)/fontforge
#	( for file in $(srcdir)/fontforge/*.h ; do if grep GGadget $$file >/dev/null 2>&1 ; then echo pointless >/dev/null ; else $(INSTALL_DATA) $$file $(DESTDIR)$(includedir)/fontforge ; fi ; done )
#
#install_docs:
#	mkdir -p $(DESTDIR)$(docdir)
#	mkdir -p $(DESTDIR)$(docdir)/flags $(DESTDIR)$(docdir)/ja
##explicit use of sh to make sure we don't invoke csh with different "if" syntax
#	-sh -c "if test -d $(srcdir)/htdocs/ja >/dev/null ; then mkdir -p $(DESTDIR)$(docdir)/ja ; fi"
#	-$(INSTALL_DATA) $(srcdir)/htdocs/*.{html,png,gif,pdf} $(DESTDIR)$(docdir)
#	-$(INSTALL_DATA) $(srcdir)/htdocs/flags/* $(DESTDIR)$(docdir)/flags
#	-$(INSTALL_DATA) $(srcdir)/htdocs/ja/* $(DESTDIR)$(docdir)/ja
#
#install_pycontrib:
#	mkdir -p $(DESTDIR)$(pythondir)
#	( cd pycontrib ; $(INSTALL_DATA) *.py $(DESTDIR)$(pythondir) )
#
#install_prog:
#	mkdir -p $(DESTDIR)$(bindir)
#	mkdir -p $(DESTDIR)$(mandir)/man1
#	mkdir -p $(DESTDIR)$(sharedir)
#	mkdir -p $(DESTDIR)$(pixmapsdir)
#	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/fontforge $(DESTDIR)$(bindir)
#	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/sfddiff $(DESTDIR)$(bindir)
#	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/fontimage $(DESTDIR)$(bindir)
#	$(LIBTOOL) --mode=install $(INSTALL) -c fontforge/fontlint $(DESTDIR)$(bindir)
##	-rm -f $(DESTDIR)$(bindir)/pfaedit
##	(cd $(DESTDIR)$(bindir) ; ln -s fontforge pfaedit)
##	$(INSTALL_DATA) fontforge/pfaedit-*.ui $(DESTDIR)$(sharedir)
##The next line will fail for most installs. If you have downloaded the cidmaps
## this will install them. If you haven't it won't. You only need the cidmaps
## if you are editing CID keyed files (and even then you can live without them
#	-$(INSTALL_DATA) $(srcdir)/cidmap/*.cidmap $(DESTDIR)$(sharedir) >/dev/null 2>&1
#	-$(INSTALL_DATA) $(srcdir)/fontforge/pixmaps/*.png $(DESTDIR)$(pixmapsdir)
#
#install: $(DESTDIR)$(sharedir) all install_libs install_incs install_po install_prog install_pycontrib
## You may need to be root to do the install
##explicit use of sh to make sure we don't invoke csh with different "if" syntax
#	sh -c "if test -d $(srcdir)/htdocs >/dev/null ; then $(MAKE) install_docs ; fi"
#	sh -c "if test -d fontforge/FontForge.app >/dev/null ; then rm -rf /Applications/FontForge.app ; mv fontforge/FontForge.app /Applications ; ln -s /usr/local/bin/fontforge /Applications/FontForge.app/Contents/MacOS/FontForge ; fi"
#	sh < systemspecific
##we don't always create a setup.py
#	-(cd pyhook ; if test -r setup.py ; then python setup.py install --prefix=$(prefix) --root=$(DESTDIR); for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done ; fi )
#
#install_py: $(DESTDIR)$(sharedir) all install_libs install_incs install_po install_prog install_pycontrib
## You may need to be root to do the install
##explicit use of sh to make sure we don't invoke csh with different "if" syntax
#	sh -c "if test -d $(srcdir)/htdocs >/dev/null ; then $(MAKE) install_docs ; for dir in * ; do if test -d $$dir ; then  rm -rf $$dir ; fi; done ; fi"
#	sh -c "if test -d fontforge/FontForge.app >/dev/null ; then rm -rf /Applications/FontForge.app ; mv fontforge/FontForge.app /Applications ; ln -s /usr/local/bin/fontforge /Applications/FontForge.app/Contents/MacOS/FontForge ; fi"
#	sh < systemspecific
#	-(cd pyhook ; python setup.py bdist --formats=gztar ; cp dist/fontforge-*.tar.gz $(DESTDIR)/tmp; for dir in * ; do if test -d $$dir -a "$$dir" != "CVS" ; then  rm -rf $$dir ; fi; done )
#
#install_gtk: fontforgegtk
#	(cd gtkui ; $(MAKE) install )
#
#uninstall:
#	-rm $(DESTDIR)$(bindir)/fontforge
#	-rm $(DESTDIR)$(bindir)/sfddiff
#	-rm $(DESTDIR)$(bindir)/fontimage
#	-rm $(DESTDIR)$(bindir)/fontlint
#	-rm $(DESTDIR)$(sharedir)/pfaedit-*.ui
#	-rm $(DESTDIR)$(localedir)/*/LC_MESSAGES/FontForge.mo
#	-rm $(DESTDIR)$(libdir)/libgdraw.*
#	-rm $(DESTDIR)$(libdir)/libgutils.*
#	-rm $(DESTDIR)$(libdir)/libgioftp.*
#	-rm $(DESTDIR)$(libdir)/libgunicode.*
#	-rm $(DESTDIR)$(libdir)/libfontforge.*
#	-rm $(DESTDIR)$(sharedir)/*.cidmap
#	-rm $(DESTDIR)$(includedir)/fontforge/*.h
#	-rm -rf $(DESTDIR)$(docdir)/*
#	-rm $(DESTDIR)$(libdir)/pkgconfig/fontforge.pc
#	-rm $(DESTDIR)$(pythondir)/*
