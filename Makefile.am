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
