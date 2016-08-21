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

noinst_LIBRARIES = libsrc.a
libsrc_a_SOURCES = \
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

libsrc_a_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_a_CFLAGS = $(AM_CFLAGS)
libsrc_a_CXXFLAGS = $(AM_CXXFLAGS)

###
### Libs to contain multiversioned functions.
###

# SSE2
if HAVE_CXXFLAGS_EXT_X86_64_SSE2
noinst_LIBRARIES += libsrc_sse2.a
libsrc_sse2_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse2_a_CPPFLAGS = $(AM_CPPFLAGS) $(CXXFLAGS_EXT_X86_64_SSE2)
libsrc_sse2_a_CFLAGS = $(AM_CFLAGS) $(CXXFLAGS_EXT_X86_64_SSE2)
libsrc_sse2_a_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE2)
#endif

# SSE4.2 but no POPCNT.
if HAVE_CXXFLAGS_EXT_X86_64_SSE4_2
noinst_LIBRARIES += libsrc_sse4_2_no_popcnt.a
libsrc_sse4_2_no_popcnt_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_no_popcnt_a_CPPFLAGS = $(AM_CPPFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(UCG_CXXFLAGS_NO_POPCNT)
libsrc_sse4_2_no_popcnt_a_CFLAGS = $(AM_CFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(UCG_CXXFLAGS_NO_POPCNT)
libsrc_sse4_2_no_popcnt_a_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(UCG_CXXFLAGS_NO_POPCNT)
endif

# SSE4.2 with POPCNT
if HAVE_CXXFLAGS_EXT_X86_64_SSE4_2
noinst_LIBRARIES += libsrc_sse4_2_popcnt.a
libsrc_sse4_2_popcnt_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_popcnt_a_CPPFLAGS = $(AM_CPPFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(UCG_CXXFLAGS_POPCNT)
libsrc_sse4_2_popcnt_a_CFLAGS = $(AM_CFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(UCG_CXXFLAGS_POPCNT)
libsrc_sse4_2_popcnt_a_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(UCG_CXXFLAGS_POPCNT)
endif

