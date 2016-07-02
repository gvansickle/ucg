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

SUBDIRS = libext

noinst_LIBRARIES = libsrc.a libsrc_sse4_2_no_popcnt.a libsrc_sse4_2_popcnt.a
libsrc_a_SOURCES = \
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

# SSE4.2 but no POPCNT. 
libsrc_sse4_2_no_popcnt_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_no_popcnt_a_CPPFLAGS = $(AM_CPPFLAGS) $(UCG_CXXFLAGS_SSE4_2) $(UCG_CXXFLAGS_NO_POPCNT)
libsrc_sse4_2_no_popcnt_a_CFLAGS = $(AM_CFLAGS) $(UCG_CXXFLAGS_SSE4_2) $(UCG_CXXFLAGS_NO_POPCNT)
libsrc_sse4_2_no_popcnt_a_CXXFLAGS = $(AM_CXXFLAGS) $(UCG_CXXFLAGS_SSE4_2) $(UCG_CXXFLAGS_NO_POPCNT)

# SSE4.2 with POPCNT
libsrc_sse4_2_popcnt_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_popcnt_a_CPPFLAGS = $(AM_CPPFLAGS) $(UCG_CXXFLAGS_SSE4_2) $(UCG_CXXFLAGS_POPCNT)
libsrc_sse4_2_popcnt_a_CFLAGS = $(AM_CFLAGS) $(UCG_CXXFLAGS_SSE4_2) $(UCG_CXXFLAGS_POPCNT)
libsrc_sse4_2_popcnt_a_CXXFLAGS = $(AM_CXXFLAGS) $(UCG_CXXFLAGS_SSE4_2) $(UCG_CXXFLAGS_POPCNT)
