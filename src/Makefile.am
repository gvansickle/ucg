# Second-level Makefile.am for UniversalCodeGrep, ./src directory.
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

SUBDIRS = future libext

# Need these to be defined here because we'll be appending to them later.
EXTRA_LTLIBRARIES =
MOSTLYCLEANFILES =

# Add suffix for Function MultiVersion libtool libraries.
SUFFIXES = .fmv.la


noinst_LTLIBRARIES = libsrc.la
libsrc_la_SOURCES = \
	ArgParse.cpp ArgParse.h \
	DirInclusionManager.cpp DirInclusionManager.h \
	Globber.cpp Globber.h \
	Logger.cpp Logger.h \
	Match.cpp Match.h \
	MatchList.cpp MatchList.h \
	File.cpp File.h \
	FileScanner.cpp FileScanner.h \
	FileScannerCpp11.cpp FileScannerCpp11.h \
	FileScannerPCRE.cpp FileScannerPCRE.h \
	FileScannerPCRE2.cpp FileScannerPCRE2.h \
	OutputContext.cpp OutputContext.h \
	OutputTask.cpp OutputTask.h \
	ResizableArray.h \
	sync_queue.h \
	sync_queue_impl_selector.h \
	TypeManager.cpp TypeManager.h

libsrc_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_la_CFLAGS = $(AM_CFLAGS)
libsrc_la_CXXFLAGS = $(AM_CXXFLAGS)
libsrc_la_LIBADD =



###
### Libs and Automake mechanisms to handle multiversioned functions in a portable manner.
###

# When compiling translation units for function multiversioning, we need to
# make sure there are no "-march=" flags on the compiler's command line.  Since the
# GNU Build System is set up to always let the user have the last word via $CFLAGS/$CXXFLAGS,
# we need to go to some extra effort here to ensure that the user's flags don't break the compilation
# of these files.
# We essentially do two things:
#
# 1. Create a new make variable, CXXFLAGS_MULTIVERSION, which is equal to the user's CXXFLAGS, but with
#    any options which would change the ISA and/or extensions required for these functions filtered out.
# 2. Build the source file for a given ISA+extension set by invoking $(MAKE) on this same Makefile,
#    but passing CXXFLAGS_MULTIVERSION for CXXFLAGS.
#
# #1 is failrly trivial, but #2 is quite a bit more involved.  We have to make sure dependencies are taken into account,
# make sure the first-stage .la library is *not* automatically built, etc.

# Strip out -march='s, -msseN's, -mavx's, etc. from the C/CXXFLAGS.  This would be easier if GNU and BSD could
# agree on basic Makefile syntax.
# GNU make only: CXXFLAGS_MULTIVERSION = $(filter-out -march=%,$(CXXFLAGS))
STRIP_ARCH_FLAGS = ${ESED} 's/-m((arch=)|(sse)|(avx))[^[:space:]]*//g'
CPPFLAGS_MULTIVERSION = $$(echo $(CPPFLAGS) | $(STRIP_ARCH_FLAGS))
CFLAGS_MULTIVERSION = $$(echo $(CFLAGS) | $(STRIP_ARCH_FLAGS))
CXXFLAGS_MULTIVERSION = $$(echo $(CXXFLAGS) | $(STRIP_ARCH_FLAGS))

# The make rule and actions for building .fmv.la libraries from .la libraries,
# and the .la libraries from their normal dependencies.
# We run these targets unconditionally, since I can't find a way to make the .fmv.la depend on the .la file's dependencies
# without causing the .la to get built with the original CXXFLAGS.
## @todo Figure out a way to hook in the dependencies of the .la to the .fmv.la file, if it's possible.
## What doesn't work:
##   - Can't depend on $(libsrc_sse2_la_OBJECTS) here, that causes libsrc_sse2_la's objects (expected) and then the .la itself
##     (that seems incorrect, nothing is depending on the .la) to be built with the original CXXFLAGS before this target is built. 
##   - Can't just depend on the _la_SOURCES, since that won't pick up the dependencies on the headers.
#
FORCE:
$(libsrc_la_LIBADD): FORCE
	@echo "MULTIVERSIONING BUILD START: Building $@"
	$(AM_V_at) $(MAKE) "CPPFLAGS=$(CPPFLAGS_MULTIVERSION)" "CFLAGS=$(CFLAGS_MULTIVERSION)" "CXXFLAGS=$(CXXFLAGS_MULTIVERSION)" $(@:.fmv.la=.la)
	$(AM_V_CXXLD) $(CXXLINK) $(@:.fmv.la=.la)
	@echo "MULTIVERSIONING BUILD END: Built $@"


# SSE2
if BUILD_CXXFLAGS_EXT_X86_64_SSE2
# From the Automake manual:
# "Convenience libraries listed in noinst_LTLIBRARIES are always built.
# Those listed in check_LTLIBRARIES are built only upon ‘make check’.
# Finally, libraries listed in EXTRA_LTLIBRARIES are never built explicitly:
# Automake outputs rules to build them, but if the library does not appear as a Makefile dependency anywhere it won’t be built."
EXTRA_LTLIBRARIES += libsrc_sse2.la
libsrc_la_LIBADD += libsrc_sse2.fmv.la
# EXTRA_LTLIBRARIES don't get cleaned, so we have to add this lib here manually.
MOSTLYCLEANFILES += libsrc_sse2.fmv.la libsrc_sse2.la
endif
libsrc_sse2_la_SOURCES = FileScanner_sse4_2.cpp
libsrc_sse2_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse2_la_CFLAGS = $(AM_CFLAGS)
libsrc_sse2_la_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE2)


# SSE4.2 but no POPCNT.
if BUILD_CXXFLAGS_EXT_X86_64_SSE4_2
if BUILD_CXXFLAGS_EXT_X86_64_NO_POPCNT
EXTRA_LTLIBRARIES += libsrc_sse4_2_no_popcnt.la
libsrc_la_LIBADD += libsrc_sse4_2_no_popcnt.fmv.la
MOSTLYCLEANFILES += libsrc_sse4_2_no_popcnt.fmv.la libsrc_sse4_2_no_popcnt.la
endif
endif
libsrc_sse4_2_no_popcnt_la_SOURCES = FileScanner_sse4_2.cpp
libsrc_sse4_2_no_popcnt_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse4_2_no_popcnt_la_CFLAGS = $(AM_CFLAGS)
libsrc_sse4_2_no_popcnt_la_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(CXXFLAGS_EXT_X86_64_NO_POPCNT)


# SSE4.2 with POPCNT
if BUILD_CXXFLAGS_EXT_X86_64_SSE4_2
if BUILD_CXXFLAGS_EXT_X86_64_POPCNT
EXTRA_LTLIBRARIES += libsrc_sse4_2_popcnt.la
libsrc_la_LIBADD += libsrc_sse4_2_popcnt.fmv.la
MOSTLYCLEANFILES += libsrc_sse4_2_popcnt.fmv.la libsrc_sse4_2_popcnt.la
endif
endif
libsrc_sse4_2_popcnt_la_SOURCES = FileScanner_sse4_2.cpp
libsrc_sse4_2_popcnt_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse4_2_popcnt_la_CFLAGS = $(AM_CFLAGS)
libsrc_sse4_2_popcnt_la_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(CXXFLAGS_EXT_X86_64_POPCNT)

