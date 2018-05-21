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

# programs to create cidmaps
noinst_PROGRAMS = mapcns1 mapgb1 mapjapan1 mapjapan2 mapkorean

EXTRA_DIST = README mapcns1.c mapgb1.c mapjapan1.c mapjapan2.c mapkorean.c

AM_CFLAGS = $(WARN_CFLAGS) "-I$(top_builddir)/inc" "-I$(top_srcdir)/inc" $(MY_CFLAGS)
DEPS = $(top_builddir)/Unicode/libgunicode.la
LDADDS = $(top_builddir)/Unicode/libgunicode.la $(MY_LIBS)

mapcns1_SOURCES = mapcns1.c
mapcns1_LDADD = $(LDADDS)

mapgb1_SOURCES = mapgb1.c
mapgb1_LDADD = $(LDADDS)

mapjapan1_SOURCES = mapjapan1.c
mapjapan1_LDADD = $(LDADDS)

mapjapan2_SOURCES = mapjapan2.c
mapjapan2_LDADD = $(LDADDS)

mapkorean_SOURCES = mapkorean.c
mapkorean_LDADD = $(LDADDS)

-include $(top_srcdir)/git.mk
