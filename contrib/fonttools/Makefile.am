# Copyright (C) 2000-2012 by George Williams
# Copyright (C) 2012 by Barry Schwartz
# Copyright (C) 2018 by Jose Da Silva
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

#--------------------------------------------------------------------------
# Small programs some people may find useful. These are built and installed
# if user runs './configure --enable-fontforge-extras' before running make.
# NOTE: some of these programs might not have been updated in a long while.

AM_CFLAGS = $(WARN_CFLAGS)

bin_PROGRAMS = acorn2sfd dewoff woff findtable pcl2ttf pfadecrypt	\
	rmligamarks showttf stripttc ttf2eps

man_MANS = acorn2sfd.1 showttf.1 ttf2eps.1

EXTRA_DIST = acorn2sfd.c acorn2sfd.1 dewoff.c woff.c findtable.c	\
	pcl2ttf.c pfadecrypt.c rmligamarks.c showttf.c showttf.1	\
	stripttc.c ttf2eps.c ttf2eps.1 portablecompositor.cpp		\
	acorn2sfd.1 showttf.1 ttf2eps.1

acorn2sfd_SOURCES = acorn2sfd.c
acorn2sfd_CFLAGS = $(MY_CFLAGS)
acorn2sfd_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	"-I$(top_builddir)/lib" "-I$(top_srcdir)/lib"			\
	"-I$(top_builddir)/fontforge" "-I$(top_srcdir)/fontforge"	\
	"-I$(top_builddir)/gutils" "-I$(top_srcdir)/gutils"		\
	"-I$(top_builddir)/Unicode" "-I$(top_srcdir)/Unicode"		\
	$(MY_CFLAGS)
acorn2sfd_LDADD = $(top_builddir)/fontforge/libfontforge.la		\
	$(top_builddir)/Unicode/libgunicode.la $(MY_LIBS)

dewoff_SOURCES = dewoff.c
dewoff_LDADD = $(ZLIB_LIBS)
woff_SOURCES = woff.c
woff_LDADD = $(ZLIB_LIBS)

findtable_SOURCES = findtable.c

pcl2ttf_SOURCES = pcl2ttf.c

pfadecrypt_SOURCES = pfadecrypt.c

rmligamarks_SOURCES = rmligamarks.c

showttf_SOURCES = showttf.c

stripttc_SOURCES = stripttc.c

ttf2eps_SOURCES = ttf2eps.c
ttf2eps_CFLAGS = $(MY_CFLAGS)
ttf2eps_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	"-I$(top_builddir)/Unicode" "-I$(top_srcdir)/Unicode"		\
	"-I$(top_builddir)/gutils" "-I$(top_srcdir)/gutils" $(MY_CFLAGS)
ttf2eps_LDADD = $(top_builddir)/Unicode/libgunicode.la			\
	$(top_builddir)/gutils/libgutils.la $(MY_LIBS)

-include $(top_srcdir)/git.mk
