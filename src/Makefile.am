# Second-level Makefile.am for UniversalCodeGrep, ./src directory.
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

noinst_LIBRARIES = libsrc.a 
libsrc_a_SOURCES = \
	ArgParse.cpp ArgParse.h \
	DirInclusionManager.cpp DirInclusionManager.h \
	Globber.cpp Globber.h \
	Match.cpp Match.h \
	MatchList.cpp MatchList.h \
	File.cpp File.h \
	FileScanner.cpp FileScanner.h \
	OutputTask.cpp OutputTask.h \
	sync_queue.h \
	sync_queue_impl_selector.h \
	TypeManager.cpp TypeManager.h

libsrc_a_CPPFLAGS = $(AM_CPPFLAGS)
libsrc_a_CFLAGS = $(AM_CFLAGS)
libsrc_a_CXXFLAGS = $(AM_CXXFLAGS)
