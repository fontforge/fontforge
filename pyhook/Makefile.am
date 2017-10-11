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

pyexec_LTLIBRARIES = psMat.la fontforge.la

psMat_la_SOURCES = psMatpyhook.c
psMat_la_LIBADD = $(LIBADD)
psMat_la_LDFLAGS = $(MY_LIB_LDFLAGS) -module -shared -avoid-version \
                   $(PYHOOK_FILE_EXTENSION)

fontforge_la_SOURCES = fontforgepyhook.c
fontforge_la_LIBADD = $(LIBADD)
fontforge_la_LDFLAGS = $(MY_LIB_LDFLAGS) -module -shared -avoid-version \
                       $(PYHOOK_FILE_EXTENSION)

AM_CFLAGS = $(WARN_CFLAGS)

AM_CPPFLAGS = -DLIBDIR='"$(libdir)"' $(PYTHON_CFLAGS)			\
	"-I$(top_builddir)/inc" "-I$(top_srcdir)/inc"			\
	"-I$(top_builddir)/lib" "-I$(top_srcdir)/lib"			\
	"-I$(top_builddir)/fontforge" "-I$(top_srcdir)/fontforge"	\
	$(MY_CFLAGS)

LIBADD = $(top_builddir)/Unicode/libgunicode.la	\
	$(top_builddir)/gutils/libgutils.la			\
	$(top_builddir)/fontforge/libfontforge.la

-include $(top_srcdir)/git.mk

install-exec-local:
	@ sfff=`(pyff_count=0 ; for item in \`echo -e "import sys\nfor file in sys.path:\n\tprint \"%s\" %file\n" | python\` ; do if [ -e "$$item"/fontforge.so ] ; then echo "$$item"/fontforge.so ; pyff_count=\`expr $$pyff_count + 1\`; fi ; done ;) | grep -v '^$(DESTDIR)$(pyexecdir)' | grep -v '^$(pyexecdir)'`; if [ "$$sfff" != "" ]; then echo "There may be another FontForge Python library on this system."; echo "$$sfff"; fi;

