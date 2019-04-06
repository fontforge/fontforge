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


MAINTAINERCLEANFILES=fontforge-config.h.in
DISTCLEANFILES=fontforge-config.h stamp-h1 stamp-h2

# FIXME: The installed headers need to be reworked for better system
# installation. In particular, they ought to have <> instead of "" and
# they should require a "fontforge/" prefix.

pkginclude_HEADERS = basics.h chardata.h charset.h dynamic.h	\
	fileutil.h gdraw.h gfile.h ggadget.h gicons.h gimage.h gio.h		\
	gkeysym.h gprogress.h gresedit.h gresource.h gwidget.h gwwiconv.h	\
	intl.h ustring.h utype.h dlist.h carbon.h gnetwork.h gutils.h		\
	ffglib.h ffgdk.h

nodist_pkginclude_HEADERS = fontforge-config.h

-include $(top_srcdir)/git.mk
