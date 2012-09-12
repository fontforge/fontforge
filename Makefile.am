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

#--------------------------------------------------------------------------

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
include $(top_srcdir)/mk/layout.am

#--------------------------------------------------------------------------

SUBDIRS = inc
SUBDIRS += Unicode gutils

if GRAPHICAL_USER_INTERFACE
SUBDIRS += gdraw
endif GRAPHICAL_USER_INTERFACE

SUBDIRS += plugins
SUBDIRS += fontforge po htdocs pycontrib
SUBDIRS += cidmap
SUBDIRS += tests

if PYTHON_EXTENSION
SUBDIRS += pyhook
endif PYTHON_EXTENSION

if MACINTOSH
SUBDIRS += mackeys
endif MACINTOSH

if DESKTOP
SUBDIRS += desktop
endif DESKTOP

if FONTTOOLS
SUBDIRS += fonttools
endif FONTTOOLS

nodist_pkgconfig_DATA = libfontforge.pc libfontforgeexe.pc

EXTRA_DIST += LICENSE README-Mac.html README-MS.html README-unix	\
	README-Unix.html Readme.VMS

uninstall-local:
	(cd desktop; $(MAKE) $(AM_MAKEFLAGS) uninstall)

#--------------------------------------------------------------------------

INDENT_DIRS = Unicode gutils gdraw fontforge pyhook tests fonttools

INDENT_PROFILE = \
	--k-and-r-style \
	--line-length110 \
	--indent-level4 \
	--no-tabs \
	--blank-lines-after-procedures \
	--no-blank-lines-after-declarations \
	--blank-before-sizeof \
	--space-after-cast \
	--space-after-for \
	--space-after-if \
	--space-after-while \
	--space-special-semicolon \
	--no-space-after-function-call-names \
	--no-space-after-parentheses \
	--dont-break-procedure-type \
	--dont-break-function-decl-args \
	--no-blank-lines-after-commas \
	--break-after-boolean-operator \
	--dont-line-up-parentheses \
	--paren-indentation4 \
	--braces-on-if-line \
	--braces-on-func-def-line \
	--braces-on-struct-decl-line \
	--case-brace-indentation4 \
	--declaration-indentation1 \
	--cuddle-do-while \
	--cuddle-else \
	--honour-newlines \
	--preprocessor-indentation1 \
	--case-indentation0 \
	--continue-at-parentheses \
	--leave-preprocessor-space \
	--dont-format-comments \
	-T\ FILE \
	-T\ AnchorPoint \
	-T\ BDFFont \
	-T\ MMSet \
	-T\ SplineChar \
	-T\ SplineFont

write-indent-profiles:
	@for dir in $(INDENT_DIRS); do \
		rm -f "$(top_srcdir)/$${dir}/.indent.pro"; \
		echo rm -f "$(top_srcdir)/$${dir}/.indent.pro"; \
		for flag in $(value INDENT_PROFILE); do \
			echo "" "$${flag}" >> "$(top_srcdir)/$${dir}/.indent.pro"; \
			echo echo '""' "$${flag}" ">>" "$(top_srcdir)/$${dir}/.indent.pro"; \
		done; \
	done

#--------------------------------------------------------------------------

-include $(top_srcdir)/git.mk
