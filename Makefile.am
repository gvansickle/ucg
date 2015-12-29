# Top-level Makefile.am for UniversalCodeGrep
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

# Make sure autotools pick up the M4 macros in the ./m4 directory, and also copy
# any third-party macros (e.g. the system-wide ax_* Autoconf Macro Archive ones
# in /usr/share/aclocal) to ./m4 so they get distributed and automatically updated.
# Only useful in the top-level Makefile.am (see http://sources.redhat.com/automake/automake.html#Rebuilding).
ACLOCAL_AMFLAGS = -I m4 --install

# The subdirectories containing the source code, docs, and tests.
# Note: The code in src depends on the libraries in third_party, so make sure the latter is always listed before the former.
# Note that we can't list these other libraries in e.g. *_DEPENDENCIES because that replaces all Automake-generated dependencies.
SUBDIRS = third_party src tests

# Make sure README.rd and other docs get distributed and installed correctly.
dist_doc_DATA = README.md COPYING AUTHORS

# The Automake rules for the ucg executable.
bin_PROGRAMS=ucg
ucg_SOURCES=main.cpp

if BUILD_BOOST_FROM_TARBALL
# Collect all the Boost libraries we need into a single variable.
BOOST_LOCAL_LIB = $(top_builddir)/third_party/$(BOOST_TARBALL_DIRNAME)/stage/lib
ALLBOOSTLIBS = $(BOOST_LOCAL_LIB)/libboost_thread.a \
	$(BOOST_LOCAL_LIB)/libboost_system.a
endif

ucg_CPPFLAGS = -I $(top_srcdir)/src \
	$(BOOST_TR1_CPPFLAGS) $(BOOST_CPPFLAGS) \
	$(AM_CPPFLAGS) 
ucg_CFLAGS = $(AM_CFLAGS)
ucg_CXXFLAGS = $(AM_CXXFLAGS)
ucg_LDFLAGS = $(BOOST_LIBTOOL_FLAGS) $(BOOST_LDFLAGS) \
	$(AM_LDFLAGS)
ucg_LDADD = ./src/libsrc.a $(ALLBOOSTLIBS)

