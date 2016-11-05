# Third-level Makefile.am for UniversalCodeGrep, ./src/future directory.
#
# Copyright 2016 Gary R. Van Sickle (grvs@users.sourceforge.net).
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

# This directory sort of parallels Python's "from __future__ import ..." functionality ("future statements").
# I'm collecting things here which are in C++14, C++17, etc., but which I really don't want to wait for.
# Obviously this doesn't include language syntax changes, but mainly new library functions, templates, etc.
# Also, I'm putting C++11 backfill here (e.g. the spotty std::to_string() support of not-so-ancient std <string> libs). 

noinst_LTLIBRARIES = libfuture.la 
libfuture_la_SOURCES = \
	dummy.cpp \
	memory.hpp \
	string.hpp \
	string_view.hpp \
	type_traits.hpp

libfuture_la_CPPFLAGS = -I $(srcdir)/../libext $(AM_CPPFLAGS)
libfuture_la_CFLAGS = $(AM_CFLAGS)
libfuture_la_CXXFLAGS = $(AM_CXXFLAGS)