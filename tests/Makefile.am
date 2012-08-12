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

EXTRA_DIST = testsuite.at $(srcdir)/package.m4 $(TESTSUITE) atlocal.in
TESTSUITE = $(srcdir)/testsuite

check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)

installcheck-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' AUTOTEST_PATH='$(bindir)' $(TESTSUITEFLAGS)

clean-local:
	test ! -f '$(TESTSUITE)' || $(SHELL) '$(TESTSUITE)' --clean

AUTOM4TE = $(SHELL) $(srcdir)/build-aux/missing --run autom4te
AUTOTEST = $(AUTOM4TE) --language=autotest
$(TESTSUITE): $(srcdir)/testsuite.at $(srcdir)/package.m4
	$(AUTOTEST) -I '$(srcdir)' -o $@.tmp $@.at
	mv $@.tmp $@

EXTRA_DIST += findoverlapbugs.py test0001.py test0101.py test1001.py	\
	test1002.py test1003.py test1004.py test1005.py test1006.py			\
	test1007.py

EXTRA_DIST += fonts/AddExtremaTest2.sfd fonts/ayn+meem.init.svg		\
	fonts/dvng10a-crash.sfd fonts/NumberPoints.sfd					\
	fonts/SplineOverlapBug1.sfd fonts/AddExtremumTest.sfd			\
	fonts/Caliban.sfd fonts/feta20.pfb fonts/OmittedCharBugs.sfd	\
	fonts/StrokeTests.sfd fonts/AHBugs.sfd fonts/CaslonMM.sfd		\
	fonts/FormalScript.sfd fonts/OverlapBugs.sfd fonts/VKern.sfd	\
	fonts/AmbrosiaBold.sfd fonts/DataURI.sfd fonts/Hinting.sfd		\
	fonts/QuadOverlapBugs.sfd fonts/AmbrosiaItalic.sfd				\
	fonts/DejaVuSerif.sfd fonts/ItalicHand.sfd						\
	fonts/QuadraticConversionBug.sfd fonts/Ambrosia.sfd				\
	fonts/DirectionTest.sfd fonts/NimbusLGCUni-Regular.sfd			\
	fonts/SimplifyBugs.sfd

DISTCLEANFILES = atconfig
