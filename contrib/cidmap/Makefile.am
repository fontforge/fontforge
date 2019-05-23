# Copyright (C) 2018 Jose Da Silva
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see http://www.gnu.org/licenses/.

# Using "fontforge-extras" as a location map, we place these here:
# NOTE: default "user" install is to /usr/local/* not /usr/* unless
#	user adds -prefix=/usr during ./configure
# /usr/bin/acorn2sfd
# /usr/bin/findtable
# /usr/bin/pcl2ttf
# /usr/bin/pfadecrypt
# /usr/bin/rmligamarks
# /usr/bin/showttf
# /usr/bin/ttf2eps
# /usr/share/fontforge/Adobe-CNS1-5.cidmap
# /usr/share/fontforge/Adobe-GB1-5.cidmap
# /usr/share/fontforge/Adobe-Identity-0.cidmap
# /usr/share/fontforge/Adobe-Japan1-6.cidmap
# /usr/share/fontforge/Adobe-Japan2-0.cidmap
# /usr/share/fontforge/Adobe-Korea1-2.cidmap
# /usr/share/man/man1/acorn2sfd.1.gz
# /usr/share/man/man1/showttf.1.gz
# /usr/share/man/man1/ttf2eps.1.gz
#
# /usr/share/fontforge/doc/Encodings.ps.gz (permanent in html)

SUBDIRS =
if FONTTOOLS
SUBDIRS += src
endif
DIST_SUBDIRS = src

cmaps = Adobe-CNS1-6.cidmap	\
	Adobe-GB1-5.cidmap	\
	Adobe-Identity-0.cidmap	\
	Adobe-Japan1-5.cidmap	\
	Adobe-Japan1-6.cidmap	\
	Adobe-Japan1-7.cidmap	\
	Adobe-Japan2-0.cidmap	\
	Adobe-Korea1-2.cidmap

EXTRA_DIST = $(cmaps)

cidmapdir =
cidmap_DATA =
if FONTTOOLS
cidmapdir += $(pkgdatadir)
cidmap_DATA += $(cmaps)
endif

-include $(top_srcdir)/git.mk
