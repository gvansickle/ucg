# Second-level Makefile.am for UniversalCodeGrep, ./third_party directory.
#
# Copyright 2015-2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# This is a proxy Makefile.am which dispatches the Autotools rules to the real makefiles of
# the third-party libraries/tools we may need.

## Much of this directory is conditionalized at configure time, so we define some vars at the top
## which we'll append to later in the sections which are not conditionalized out.

noinst_SCRIPTS =

# Directory trees we need to delete on a "make clean".
CLEAN_LOCAL_DIRS = 

# The one file we explicitly "build" is this stampfile.
# This stampfile depends on all files added to $(third_party_complete_deps), which will in turn be the result of
# the various sub-builds.  We do it this way (instead of listing all the sub-build stamps here) so we can still
# benefit from parallel builds.
BUILT_SOURCES = stampfile-all-third-party-builds-complete

# Append any stamp (or other) files which indicate that a sub-build is complete to this variable.
third_party_complete_deps =

# Extra files we'll need to distribute.
EXTRA_DIST =

# Files to clean for "make mostlyclean" and "make clean".
MOSTLYCLEANFILES =
CLEANFILES =

# Libtool libraries only needed for "make check".
check_LTLIBRARIES =


###
### The Lean Mean C++ Option Parser
###

LMCPPOP_RELEASE_NO = 1.7

# This is a header-only library.  We just need to rig things up so it gets distributed.
EXTRA_DIST += \
	optionparser-$(LMCPPOP_RELEASE_NO)/DoxygenLayout.xml \
	optionparser-$(LMCPPOP_RELEASE_NO)/README_ARGC_ARGV \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/printUsage.h \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/example.cpp \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/testodr1.cc \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/testprintusage.cpp \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/example_arg.cc \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/testodr2.cc \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/testparse.cpp \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/Makefile \
	optionparser-$(LMCPPOP_RELEASE_NO)/src/optionparser.h \
	optionparser-$(LMCPPOP_RELEASE_NO)/Doxyfile

###
### Google Test
###

if HAVE_GOOGLETEST
# @todo Fix this, "make distcheck" wants to distribute gtest-all.cc.

third_party_complete_deps += stampfile-gtest-tarball-untarred

GT_RELEASE_NO = 1.12.1

# Directory where Google Test tarball will be extracted.
GTEST_ROOT = ./googletest-release-$(GT_RELEASE_NO)

# Tarball as downloaded from <https://github.com/google/googletest/releases>
stampfile-gtest-tarball-untarred: $(srcdir)/googletest-release-$(GT_RELEASE_NO).tar.gz
	tar -xaf "$<"
	echo "$$(date)" > $@.tmp
	mv $@.tmp $@

check_LTLIBRARIES += libgtest_all.la
libgtest_all_la_SOURCES = ./googletest-release-$(GT_RELEASE_NO)/googletest/src/gtest-all.cc
libgtest_all_la_CPPFLAGS= -I./googletest-release-$(GT_RELEASE_NO)/googletest/include -I./googletest-release-$(GT_RELEASE_NO)/googletest
libgtest_all_la_CXXFLAGS = -pthread
EXTRA_libgtest_all_la_DEPENDENCIES = stampfile-gtest-tarball-untarred
endif


###
### Include third-party Makefile.am's here.
###


# The stamp file which will depend on all active third-party builds.
# We do this this way so as to maximize parallel build potential.
stampfile-all-third-party-builds-complete: $(third_party_complete_deps)
	@echo "All third-party builds complete, $$(date)" > "$@-temp"
	@mv "$@-temp" "$@"

###
### MAINTAINER-ONLY TARGETS
### These are strictly of use for the maintainer, e.g. to update the version of a third-party lib.
###



###
### Clean rules
###

# Delete all the stamps except for the untarring ones for all clean targets.
# Also delete the generated .tar files, they're cheap to rebuild.
MOSTLYCLEANFILES +=  $(CLEANFILES_REPORT_BOILERPLATE) stampfile-all-third-party-builds-complete
	 
# We need to hook 'clean' here because we need to rm the entire untarred source trees, not just individual files.	
clean-local:
	@:; # Strip any spaces off the ends of the directory list.
	$(AM_V_GEN) DIRLIST="$$(echo "$(CLEAN_LOCAL_DIRS)" | $(ESED) 's/[ ]*(.*)[ ]*/\1\n/')"; \
	if [ -n "$${DIRLIST}" ]; then \
		for DIRNAME in $${DIRLIST}; \
		do \
			echo "Removing directory $${DIRNAME}...";\
			test ! -d "$${DIRNAME}" || rm -rf "$${DIRNAME}"; \
		done; \
	fi;

# Nothing in here to install or check, so no other *-local hooks.
