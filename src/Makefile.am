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
MOSTLYCLEANFILES =
EXTRA_LTLIBRARIES =

noinst_LTLIBRARIES = libsrc.la
libsrc_la_SOURCES = \
	ArgParse.cpp ArgParse.h \
	DirInclusionManager.cpp DirInclusionManager.h \
	Globber.cpp Globber.h \
	Logger.cpp Logger.h \
	Match.cpp Match.h \
	MatchList.cpp MatchList.h \
	File.cpp File.h \
	FileID.cpp FileID.h \
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
# - Create a new make variable, CXXFLAGS_MULTIVERSION, which is equal to the user's CXXFLAGS, but with
#   any options which would change the ISA and/or extensions required for these functions filtered out.
# - Build the source file for a given ISA+extension set by invoking $(MAKE) on this same Makefile,
#   but passing CXXFLAGS_MULTIVERSION for CXXFLAGS.
#
CXXFLAGS_MULTIVERSION = $(filter-out -march=%,$(CXXFLAGS))

libsrc_sse2.fmv.la: $(libsrc_sse2_la_SOURCES)
	@echo "##### DEBUG: Building $@ from libsrc_sse2.la"
	$(MAKE) CXXFLAGS=$(CXXFLAGS_MULTIVERSION) libsrc_sse2.la
	$(CXXLINK) libsrc_sse2.la

#CXXFLAGS_USER:=$(CXXFLAGS)
#CXXFLAGS_FILT:=$(filter-out -march=%,$(CXXFLAGS_USER))
#CXXFLAGS:=


# SSE2
if BUILD_CXXFLAGS_EXT_X86_64_SSE2
EXTRA_LTLIBRARIES += libsrc_sse2.la
libsrc_la_LIBADD += libsrc_sse2.fmv.la
MOSTLYCLEANFILES += libsrc_sse2.fmv.la libsrc_sse2.la
#noinst_LTLIBRARIES += libsrc_sse2.la
endif
libsrc_sse2_la_SOURCES = FileScanner_sse4_2.cpp
libsrc_sse2_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse2_la_CFLAGS = $(AM_CFLAGS)
libsrc_sse2_la_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE2)


# SSE4.2 but no POPCNT.
if BUILD_CXXFLAGS_EXT_X86_64_SSE4_2
if BUILD_CXXFLAGS_EXT_X86_64_NO_POPCNT
noinst_LTLIBRARIES += libsrc_sse4_2_no_popcnt.la
endif
endif
libsrc_sse4_2_no_popcnt_la_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_no_popcnt_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse4_2_no_popcnt_la_CFLAGS = $(AM_CFLAGS)
libsrc_sse4_2_no_popcnt_la_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(CXXFLAGS_EXT_X86_64_NO_POPCNT)

# SSE4.2 with POPCNT
if BUILD_CXXFLAGS_EXT_X86_64_SSE4_2
if BUILD_CXXFLAGS_EXT_X86_64_POPCNT
noinst_LTLIBRARIES += libsrc_sse4_2_popcnt.la
endif
endif
libsrc_sse4_2_popcnt_la_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_popcnt_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse4_2_popcnt_la_CFLAGS = $(AM_CFLAGS)
libsrc_sse4_2_popcnt_la_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(CXXFLAGS_EXT_X86_64_POPCNT)

