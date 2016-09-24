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

lib_LTLIBRARIES = libgunicode.la

libgunicode_la_SOURCES =	\
	charset/alphabet.c	\
	charset/big5.c		\
	charset/big5hkscs.c	\
	charset/gb2312.c	\
	charset/iso_8859_10.c	\
	charset/iso_8859_11.c	\
	charset/iso_8859_13.c	\
	charset/iso_8859_14.c	\
	charset/iso_8859_15.c	\
	charset/iso_8859_16.c	\
	charset/iso_8859_1.c	\
	charset/iso_8859_2.c	\
	charset/iso_8859_3.c	\
	charset/iso_8859_4.c	\
	charset/iso_8859_5.c	\
	charset/iso_8859_6.c	\
	charset/iso_8859_7.c	\
	charset/iso_8859_8.c	\
	charset/iso_8859_9.c	\
	charset/jis201.c	\
	charset/jis.c		\
	charset/johab.c		\
	charset/koi8_r.c	\
	charset/ksc5601.c	\
	charset/mac.c		\
	charset/MacSymbol.c	\
	charset/win.c		\
	charset/ZapfDingbats.c	\
	ArabicForms.c		\
	char.c			\
	is_Ligature.c		\
	memory.c		\
	ucharmap.c		\
	unialt.c		\
	ustring.c		\
	utype.c			\
	gwwiconv.c		\
	combiners.h		\
	$(NULL)

#	usprintf.c

libgunicode_la_CFLAGS = $(WARN_CFLAGS)
libgunicode_la_CPPFLAGS = "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"	\
	"-I$(top_builddir)/lib" "-I$(top_srcdir)/lib"			\
	$(MY_CFLAGS)

libgunicode_la_LDFLAGS = $(MY_CFLAGS) -version-info $(LIBGUNICODE_VERSION) \
	${MY_LIB_LDFLAGS} $(top_builddir)/lib/libgnu.la

libgunicode_la_LIBADD = $(MY_LIBS)

#--------------------------------------------------------------------------

#dump_SOURCES = dump.c
#makebuildtables_SOURCES = makebuildtables.c utype.c
#makeutype_SOURCES = makeutype.c

#noinst_PROGRAMS = dump makebuildtables makeutype

EXTRA_DIST = README.TXT #dump.c makebuildtables.c makeutype.c

#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
