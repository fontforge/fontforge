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

# Ignore files installed by gnulib-tool
GITIGNOREFILES = \
	.gitmodules \
	INSTALL \
	aclocal.m4 \
	config/ \
	gnulib/ \
	lib/ \
	libltdl/


#--------------------------------------------------------------------------

AM_CPPFLAGS =
AM_LDFLAGS =

BUILT_SOURCES =
EXTRA_DIST =
CLEANFILES =
MOSTLYCLEANFILES =
MAINTAINERCLEANFILES=config.h.in
DISTCLEANFILES=config.h stamp-h1

include_HEADERS =
noinst_LTLIBRARIES =
lib_LTLIBRARIES =
EXTRA_LTLIBRARIES =

include $(top_srcdir)/mk/layout.am

#--------------------------------------------------------------------------

SUBDIRS = lib inc libltdl Unicode gutils

if LIBZMQ
SUBDIRS += collab
endif

SUBDIRS += fontforge

if GRAPHICAL_USER_INTERFACE
SUBDIRS += gdraw
endif GRAPHICAL_USER_INTERFACE

SUBDIRS += fontforgeexe pyhook

if MACINTOSH
SUBDIRS += mackeys osx
endif MACINTOSH

if XDG_DESKTOP
SUBDIRS += desktop
endif XDG_DESKTOP

SUBDIRS += \
	contrib \
	doc \
	nodejs \
	plugins \
	po \
	pycontrib \
	share \
	tests \
	$(NULL)

#--------------------------------------------------------------------------

nodist_pkgconfig_DATA = libfontforge.pc libfontforgeexe.pc

EXTRA_DIST += \
	m4/gnulib-cache.m4 \
	bootstrap \
	bootstrap.conf \
	COPYING.gplv3 \
	LICENSE \
	doc/README-Mac.html \
	doc/README-MS.html \
	doc/README-unix	\
	doc/README-Unix.html \
	Packaging/FontForge-doc.spec \
	Packaging/FontForge.spec \
	Packaging/FontForge.static.spec \
	$(NULL)

#--------------------------------------------------------------------------

DISTCHECK_CONFIGURE_FLAGS = --enable-theme-2012 --enable-debug

#	\
#	--disable-native-scripting --disable-python-scripting	\
#	--without-giflib --without-libjpeg --without-libpng	\
#	--without-libtiff --without-libxml --without-libspiro	\
#	--without-libuninameslist --without-libunicodenames	\
#	--without-libzmq  --without-libreadline


#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
