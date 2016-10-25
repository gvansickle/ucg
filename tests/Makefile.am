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

EXTRA_DIST = $(TESTSUITE_AT) $(TESTSUITE) $(srcdir)/package.m4 atlocal.in \
	gen_test_script.py stats.awk \
	benchmark_progs.csv opts_defs.csv test_cases.csv \
	$(BENCHMARKSCRIPTS)
CLEANFILES = perf_test_results.txt
DISTCLEANFILES = atconfig
MAINTAINERCLEANFILES = Makefile.in $(TESTSUITE) $(BENCHMARKSCRIPTS)

###
### Dummy file generator exe.
### Only built during a "make check", and not installed.
###
check_PROGRAMS = dummy-file-gen portable_time
dummy_file_gen_SOURCES = dummy-file-gen.cpp lorem_ipsum.hpp
dummy_file_gen_CPPFLAGS = -I $(top_srcdir)/src $(AM_CPPFLAGS) 
dummy_file_gen_CFLAGS = $(AM_CFLAGS)
dummy_file_gen_CXXFLAGS = $(AM_CXXFLAGS)
dummy_file_gen_LDFLAGS = $(AM_LDFLAGS)

###
### Portable time utility.
###
portable_time_SOURCES = portable_time.cpp
portable_time_CPPFLAGS = -I $(top_srcdir)/src $(AM_CPPFLAGS) 
portable_time_CFLAGS = $(AM_CFLAGS)
portable_time_CXXFLAGS = $(AM_CXXFLAGS)
portable_time_LDFLAGS = $(AM_LDFLAGS)

###
### Benchmark scripts.  These are maintainer-built source.
###
BENCHMARKSCRIPTS = $(srcdir)/TC1.sh \
	$(srcdir)/TC2.sh \
	$(srcdir)/TC3.sh \
	$(srcdir)/TC4.sh \
	$(srcdir)/TC5.sh \
	$(srcdir)/TC6.sh

     
# This line appears in the Autoconf/Autotest manual, but see configure.ac.
# The "AM_MISSING_PROG([AUTOM4TE], [autom4te])" line there takes care of this.
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
	} > $@.temp
	mv $@.temp '$(srcdir)/package.m4'

# The .at files which make up the test suite.
TESTSUITE_AT = \
	testsuite.at \
	arg_and_rc_handling.at \
	dir_tree_traversal.at \
	file_presentation.at \
	match_correctness.at \
	type_inclusion.at \
	version.at \
	sanity.at \
	search_modifiers.at \
	performance_tests.at \
	$(BENCHMARKSCRIPTS)

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
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) ; RETVAL=$$? ; \
	if test $$RETVAL != 0; then \
		LOGS=$$(find $(builddir)/testsuite.dir -iname 'testsuite.log'); \
		for FN in $$LOGS; do \
			echo $$FN; \
			cat $$FN; \
		done; \
	fi; \
	exit $$RETVAL;
     
installcheck-local: atconfig atlocal $(TESTSUITE)
	$(SHELL) '$(TESTSUITE)' $(TESTSUITEFLAGS) AUTOTEST_PATH='$(bindir)' ; RETVAL=$$? ; \
	if test $$RETVAL != 0; then \
		LOGS=$$(find $(builddir)/testsuite.dir -iname 'testsuite.log'); \
		for FN in $$LOGS; do \
			echo $$FN; \
			cat $$FN; \
		done; \
	fi; \
	exit $$RETVAL;


###
### Rules to build benchmark test scripts at the same time testsuite is built.
### I.e. these are built, distributed files.
###

BENCHMARK_SCRIPT_DEPS = gen_test_script.py stats.awk \
	benchmark_progs.csv opts_defs.csv test_cases.csv Makefile.am

dist_check_SCRIPTS = $(BENCHMARKSCRIPTS)

# The TCn.sh rebuild rules.
# These files should only ever need to be built pre-"make dist", and should never need to be
# regenerated from a tarball.  Hence, we're using the $(MAINTPYTHON) variable here, which is either
# a python interpreter or the "missing" script.

$(srcdir)/TC1.sh : $(BENCHMARK_SCRIPT_DEPS)
	$(MAINTPYTHON) $(srcdir)/gen_test_script.py --csv-dir=$(srcdir) --test-case=TC1 $${top_srcdir}/../boost_1_58_0 -r '$${builddir}/perf_test_results.txt' -o $@ && chmod +x $@

$(srcdir)/TC2.sh : $(BENCHMARK_SCRIPT_DEPS)
	$(MAINTPYTHON) $(srcdir)/gen_test_script.py --csv-dir=$(srcdir) --test-case=TC2 $${top_srcdir}/../boost_1_58_0 -r '$${builddir}/perf_test_results.txt' -o $@ && chmod +x $@

$(srcdir)/TC3.sh : $(BENCHMARK_SCRIPT_DEPS)
	$(MAINTPYTHON) $(srcdir)/gen_test_script.py --csv-dir=$(srcdir) --test-case=TC3 "$${builddir}/500MBLoremIpsum.cpp" -r '$${builddir}/perf_test_results.txt' -o $@ && chmod +x $@

$(srcdir)/TC4.sh : $(BENCHMARK_SCRIPT_DEPS)
	$(MAINTPYTHON) $(srcdir)/gen_test_script.py --csv-dir=$(srcdir) --test-case=TC4 "$${builddir}/500MBLoremIpsum.cpp" -r '$${builddir}/perf_test_results.txt' -o $@ && chmod +x $@

$(srcdir)/TC5.sh : $(BENCHMARK_SCRIPT_DEPS)
	$(MAINTPYTHON) $(srcdir)/gen_test_script.py --csv-dir=$(srcdir) --test-case=TC5 '$${top_srcdir}/../boost_1_58_0' -r '$${builddir}/perf_test_results.txt' -o $@ && chmod +x $@

$(srcdir)/TC6.sh : $(BENCHMARK_SCRIPT_DEPS)
	$(MAINTPYTHON) $(srcdir)/gen_test_script.py --csv-dir=$(srcdir) --test-case=TC6 --opt=exclude_dir_literal=docs '$${top_srcdir}/../boost_1_58_0' -r '$${builddir}/perf_test_results.txt' -o $@ && chmod +x $@

