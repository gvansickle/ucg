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
EXTRA_LTLIBRARIES =

SUFFIXES = .fmv

noinst_LTLIBRARIES = libsrc.la ###libsrc_isa_ext_x86_64.la
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

nodist_EXTRA_libsrc_la_SOURCES =
libsrc_la_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_la_CFLAGS = $(AM_CFLAGS)
libsrc_la_CXXFLAGS = $(AM_CXXFLAGS)
libsrc_la_LIBADD =



#libsrc_isa_ext_x86_64_la_SOURCES = 


###
### Libs to contain multiversioned functions.
###
CXXFLAGS_MULTIVERSION = $(filter-out -march=%,$(CXXFLAGS))

#.fmv.lo:
libsrc_sse2.fmv.la:
	@echo "##### DEBUG: Building $@ from libsrc_sse2.la"
	$(MAKE) CXXFLAGS=$(CXXFLAGS_MULTIVERSION) libsrc_sse2.la
	$(CXXLINK) libsrc_sse2.la

#libsrc_isa_ext_x86_64.la:
#	$(MAKE) CXXFLAGS=$(CXXFLAGS_MULTIVERSION) libsrc_sse2.la
#	libtool --tag=CXX --verbose --mode=link -static -o $@ libsrc_sse2.la
#$(AR) -rcT $@ libsrc_sse2.a
#echo -e "create libsrc_isa_ext_x86_64.a\naddlib libsrc_sse2.a\nsave\nend" | ar -M

#CXXFLAGS_USER:=$(CXXFLAGS)
#CXXFLAGS_FILT:=$(filter-out -march=%,$(CXXFLAGS_USER))
#CXXFLAGS:=

#@CXXFLAGS@
#@CXXFLAGS_TESTING@
#@CXXFLAGS_TESTING_OBJ@

# SSE2
if BUILD_CXXFLAGS_EXT_X86_64_SSE2
EXTRA_LTLIBRARIES += libsrc_sse2.la
#nodist_EXTRA_libsrc_la_SOURCES += libsrc_sse2.la.sse2.fmv
libsrc_la_LIBADD += libsrc_sse2.fmv.la
MOSTLYCLEANFILES += libsrc_sse2.fmv.la
#libsrc_isa_ext_x86_64_la_DEPENDENCIES += libsrc_sse2.la
#libsrc_isa_ext_x86_64_la_SOURCES += libsrc_sse2.la 
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

