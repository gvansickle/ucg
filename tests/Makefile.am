# Second-level Makefile.am for UniversalCodeGrep's ./tests directory.
#
# Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).
#
# This file is part of UniversalCodeGrep.
#
# UniversalCodeGrep is free software: you can redistribute it and/or modify it under the
# terms of version 3 of the GNU General Public License as published by the Free
# Software Foundation.
#
# UniversalCodeGrep is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.  See the GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License along with
# UniversalCodeGrep.  If not, see <http://www.gnu.org/licenses/>.

## Process this file with automake to produce Makefile.in.

EXTRA_DIST = $(TESTSUITE_AT) $(TESTSUITE) $(srcdir)/package.m4 atlocal.in
CLEANFILES = perf_test_results.txt
DISTCLEANFILES = atconfig
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE)
     
# This line appears in the Autoconf/Autotest manual, but see configure.ac.
# The "AM_MISSING_PROG([AUTOM4TE], [autom4te])" line there takes care of this,
# though the "AM_MISSING_PROG" macro is not documented.
#AUTOM4TE = $(SHELL) $(top_srcdir)/build-aux/missing --run autom4te


# The `:;' works around a Bash 3.2 bug when the output is not writeable.
$(srcdir)/package.m4: $(top_srcdir)/configure.ac
	@ :;{ \
	echo '# Copyright 2015 Gary R. Van Sickle (grvs@users.sourceforge.net).' && \
	echo '#' && \
	echo '# This file is part of UniversalCodeGrep.' && \
	echo '#' && \
	echo '# UniversalCodeGrep is free software: you can redistribute it and/or modify it under the' && \
	echo '# terms of version 3 of the GNU General Public License as published by the Free' && \
	echo '# Software Foundation.' && \
	echo '#' && \
	echo '# UniversalCodeGrep is distributed in the hope that it will be useful, but WITHOUT ANY' && \
	echo '# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A' && \
	echo '# PARTICULAR PURPOSE.  See the GNU General Public License for more details.' && \
	echo '#' && \
	echo '# You should have received a copy of the GNU General Public License along with' && \
	echo '# UniversalCodeGrep.  If not, see <http://www.gnu.org/licenses/>.' &&\
	echo '#' && \
	echo '# THIS FILE GENERATED, DO NOT EDIT.' && \
	echo '# Signature of the current package.' && \
	echo 'm4_define([AT_PACKAGE_NAME],      [$(PACKAGE_NAME)])' && \
	echo 'm4_define([AT_PACKAGE_TARNAME],   [$(PACKAGE_TARNAME)])' && \
	echo 'm4_define([AT_PACKAGE_VERSION],   [$(PACKAGE_VERSION)$(VC_REV)])' && \
	echo 'm4_define([AT_PACKAGE_STRING],    [$(PACKAGE_STRING)$(VC_REV)])' && \
	echo 'm4_define([AT_PACKAGE_BUGREPORT], [$(PACKAGE_BUGREPORT)])' && \
	echo 'm4_define([AT_PACKAGE_URL],       [$(PACKAGE_URL)])' ; \
	} > '$(srcdir)/package.m4'

# The .at files which make up the test suite.
TESTSUITE_AT = \
	testsuite.at \
	file_presentation.at \
	type_inclusion.at \
	version.at \
	sanity.at \
	search_modifiers.at \
	performance_tests.at

# The actual shell script which is the testsuite.
TESTSUITE = $(srcdir)/testsuite

# Our AutoTest compiler is really Autom4te with "--language=autotest".
AUTOTEST = $(AUTOM4TE) --language=autotest

###
### The rule which makes the "testsuite" shell script.
### Note that this script is distributed, and ordinarily anyone using a distribution
### shouldn't need to or want to rebuild it.  "make distcheck" will fail in cryptic
### ways if it is for some reason rebuilt:
###   - "$@.tmp" points to a file in $(srcdir)/tests/, which during "make distcheck" is read-only, so autom4te fails.
###   - Similarly, "$(srcdir)/tests/testsuite" can't be overwritten, so the mv will fail.
### To avoid this, don't make $(TESTSUITE) depend on non-distributed built files.
### See <http://sources.redhat.com/automake/automake.html#distcleancheck> for more info.
###
$(TESTSUITE): $(srcdir)/package.m4 $(TESTSUITE_AT)
	$(AUTOTEST) -I '$(srcdir)' $(srcdir)/testsuite.at -o $@.tmp
	mv $@.tmp $@
	
# It appears that at least up to autoconf 2.68, atconfig doesn't get a dependency
# and rebuild rule generated for it.  Do that manually here.
atconfig: $(top_builddir)/config.status
	cd $(top_builddir) && ./config.status tests/$@

###
### Rules to hook the AutoTest functionality into Automake.
###
     
clean-local:
	test ! -f '$(TESTSUITE)' ||	$(SHELL) '$(TESTSUITE)' --clean
	
check-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS)
     
installcheck-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) AUTOTEST_PATH='$(bindir)'

