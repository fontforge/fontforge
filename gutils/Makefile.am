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

lib_LTLIBRARIES = libgutils.la libgioftp.la

#--------------------------------------------------------------------------

libgutils_la_SOURCES = divisors.c fsys.c gcol.c gimage.c				\
	gimagereadbmp.c gimageread.c gimagereadgif.c gimagereadjpeg.c		\
	gimagereadpng.c gimagereadras.c gimagereadrgb.c gimagereadtiff.c	\
	gimagereadxbm.c gimagereadxpm.c gimagewritebmp.c					\
	gimagewritegimage.c gimagewritejpeg.c gimagewritepng.c				\
	gimagewritexbm.c gimagewritexpm.c gwwintl.c gio.c giofile.c			\
	giohosts.c giomime.c giothread.c giotrans.c gimagebmpP.h			\
	giofuncP.h gioP.h dlist.c prefs.c prefs.h gnetwork.c                            \
        unicodelibinfo.c unicodelibinfo.h

libgutils_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

libgutils_la_LIBADD = $(top_builddir)/Unicode/libgunicode.la

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libgutils_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -release $(VERSION)

#--------------------------------------------------------------------------

libgioftp_la_SOURCES = gioftp.c gioftpP.h
libgioftp_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	$(MY_CFLAGS)

# Using -release instead of -version-info seems appropriate in the
# absence of a well defined ABI.
libgioftp_la_LDFLAGS = $(MY_CFLAGS) $(MY_LIBS) -release $(VERSION)

#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
