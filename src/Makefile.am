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

MOSTLYCLEANFILES =
EXTRA_LIBRARIES =

noinst_LIBRARIES = libsrc.a libsrc_isa_ext_x86_64.a
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

libsrc_isa_ext_x86_64_a_SOURCES =

###
### Libs to contain multiversioned functions.
###
CXXFLAGS_MULTIVERSION = $(filter-out -march=%,$(CXXFLAGS))
libsrc_isa_ext_x86_64.a:
	$(MAKE) CXXFLAGS=$(CXXFLAGS_MULTIVERSION) libsrc_sse2.a
	$(AR) -rcT $@ libsrc_sse2.a
	$(RANLIB) $@
#echo -e "create libsrc_isa_ext_x86_64.a\naddlib libsrc_sse2.a\nsave\nend" | ar -M

#CXXFLAGS_USER:=$(CXXFLAGS)
#CXXFLAGS_FILT:=$(filter-out -march=%,$(CXXFLAGS_USER))
#CXXFLAGS:=

#@CXXFLAGS@
#@CXXFLAGS_TESTING@
#@CXXFLAGS_TESTING_OBJ@

# SSE2
if BUILD_CXXFLAGS_EXT_X86_64_SSE2
EXTRA_LIBRARIES += libsrc_sse2.a
MOSTLYCLEANFILES += libsrc_sse2.a
#libsrc_isa_ext_x86_64_a_SOURCES += libsrc_sse2.a 
#noinst_LIBRARIES += libsrc_isa_ext_x86_64.a 
# libsrc_sse2.a
endif
libsrc_sse2_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse2_a_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse2_a_CFLAGS = $(AM_CFLAGS)
libsrc_sse2_a_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE2)


# SSE4.2 but no POPCNT.
if BUILD_CXXFLAGS_EXT_X86_64_SSE4_2
if BUILD_CXXFLAGS_EXT_X86_64_NO_POPCNT
noinst_LIBRARIES += libsrc_sse4_2_no_popcnt.a
endif
endif
libsrc_sse4_2_no_popcnt_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_no_popcnt_a_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse4_2_no_popcnt_a_CFLAGS = $(AM_CFLAGS)
libsrc_sse4_2_no_popcnt_a_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(CXXFLAGS_EXT_X86_64_NO_POPCNT)

# SSE4.2 with POPCNT
if BUILD_CXXFLAGS_EXT_X86_64_SSE4_2
if BUILD_CXXFLAGS_EXT_X86_64_POPCNT
noinst_LIBRARIES += libsrc_sse4_2_popcnt.a
endif
endif
libsrc_sse4_2_popcnt_a_SOURCES = \
	FileScanner_sse4_2.cpp

libsrc_sse4_2_popcnt_a_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_sse4_2_popcnt_a_CFLAGS = $(AM_CFLAGS)
libsrc_sse4_2_popcnt_a_CXXFLAGS = $(AM_CXXFLAGS) $(CXXFLAGS_EXT_X86_64_SSE4_2) $(CXXFLAGS_EXT_X86_64_POPCNT)

