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

# The `:;' works around a Bash 3.2 bug when the output is not writable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	:;{ \
		echo '# Signature of the current package.' && \
		echo 'm4_define([AT_PACKAGE_NAME],' && \
		echo '  [$(PACKAGE_NAME)])' && \
		echo 'm4_define([AT_PACKAGE_TARNAME],' && \
		echo '  [$(PACKAGE_TARNAME)])' && \
		echo 'm4_define([AT_PACKAGE_VERSION],' && \
		echo '  [$(PACKAGE_VERSION)])' && \
		echo 'm4_define([AT_PACKAGE_STRING],' && \
		echo '  [$(PACKAGE_STRING)])' && \
		echo 'm4_define([AT_PACKAGE_BUGREPORT],' && \
		echo '  [$(PACKAGE_BUGREPORT)])'; \
		echo 'm4_define([AT_PACKAGE_URL],' && \
		echo '  [$(PACKAGE_URL)])'; \
	} >'$(srcdir)/package.m4'

# mirror providing free access to this file
#MunhwaGothic-Bold:
#	$(WGET) "http://mirror.1000mbps.com/freebsd/ports/distfiles/MunhwaGothic-Bold" -O fonts/MunhwaGothic-Bold

noinst_FILES = * fonts/* results/* results

EXTRA_DIST = testsuite.at $(srcdir)/package.m4 $(TESTSUITE) atlocal.in
TESTSUITE = $(srcdir)/testsuite

check-local: atconfig atlocal $(TESTSUITE)
	$(MKDIR_P) $(builddir)/results
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

installcheck-local: atconfig atlocal $(TESTSUITE)
	$(MKDIR_P) $(builddir)/results
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean
	rm -fR $(builddir)/results/*

AUTOM4TE = $(SHELL) $(srcdir)/build-aux/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(srcdir)/testsuite.at $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@

# test fonts
EXTRA_DIST += fonts/AddExtremaTest2.sfd fonts/AddExtremumTest.sfd	\
	fonts/AHBugs.sfd fonts/Ambrosia.sfd fonts/AmbrosiaBold.sfd	\
	fonts/AmbrosiaItalic.sfd fonts/ayn+meem.init.svg		\
	fonts/Caliban.sfd fonts/CaslonMM.sfd fonts/DataURI.sfd		\
	fonts/DejaVuSerif.sfd fonts/DirectionTest.sfd			\
	fonts/dvng10a-crash.sfd fonts/feta20.pfb fonts/FormalScript.sfd	\
	fonts/Hinting.sfd fonts/ItalicHand.sfd				\
	fonts/NimbusLGCUni-Regular.sfd fonts/NumberPoints.sfd		\
	fonts/OmittedCharBugs.sfd fonts/OverlapBugs.sfd			\
	fonts/QuadOverlapBugs.sfd fonts/QuadraticConversionBug.sfd	\
	fonts/SimplifyBugs.sfd fonts/SplineOverlapBug1.sfd		\
	fonts/StrokeTests.sfd fonts/VKern.sfd README

# native test scripts
EXTRA_DIST += helper107.pe.in helper118A.pe.in helper118B.pe.in		\
	svg2ttf.pe.in test001.pe.in test002.pe.in test003.pe.in		\
	test004.pe.in test005.pe.in test006.pe.in test007.pe.in		\
	test008.pe.in test009.pe.in test010.pe.in test101.pe.in		\
	test102.pe.in test103.pe.in test104.pe.in test105.pe.in		\
	test106.pe.in test107.pe.in test108.pe.in test109.pe.in		\
	test110.pe.in test111.pe.in test112.pe.in test113.pe.in		\
	test114.pe.in test115.pe.in test116.pe.in test117.pe.in		\
	test118.pe.in test119.pe.in test120.pe.in test121.pe.in		\
	test122.pe.in test123.pe.in test124.pe.in test125.pe.in		\
	test126.pe.in test127.pe.in

# python test scripts
EXTRA_DIST += findoverlapbugs.py.in test0001.py.in test0101.py.in	\
	test1001.py.in test1001a.py.in test1001b.py.in test1001c.py.in	\
	test1002.py.in test1003.py.in test1004.py.in test1005.py.in	\
	test1006.py.in test1007.py.in

EXTRA_DIST += link_test.c randomtest.c runtests build-aux/missing

DISTCLEANFILES = atconfig *.pe *.py randomtest

-include $(top_srcdir)/git.mk
