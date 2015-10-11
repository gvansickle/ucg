# configure.ac for UniversalCodeGrep
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

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([UniversalCodeGrep], [0.1.0], [https://github.com/gvansickle/ucg/issues],,[https://github.com/gvansickle/ucg])

AC_CONFIG_SRCDIR([src/Globber.cpp])
AC_CONFIG_HEADERS([config.h])
AC_COPYRIGHT([Copyright (C) 2015 Gary R. Van Sickle])

# Use the auxilliary build tools (e.g. install-sh, config.sub, etc.) in ./build-aux.
AC_CONFIG_AUX_DIR([build-aux])

# Additional Autoconf macros are in ./m4.
AC_CONFIG_MACRO_DIR([m4])

# Clear the "user defaults" for compiler optimization and debug flags.  This doesn't override any configure-time or make-time settings the
# builder may specifiy for these variables, it just makes the values empty if they're not specified at configure- or make-time.
# Otherwise, Autoconf's AC_PROG_CXX et al macros assume the builder wants to compile everything "-g -O2" by default, which overrides
# any AM_C*FLAGS.
# This otherwise-frowned-upon touching of these user variables is sanctioned by the Autoconf manual for this purpose.
# See the discussion of AC_PROG_CC in http://www.gnu.org/software/autoconf/manual/autoconf.html#C-Compiler, which reads:
# "If your package does not like this default, then it is acceptable to insert the line ': ${CFLAGS=""}' after AC_INIT and before AC_PROG_CC
# to select an empty default instead."
: ${CPPFLAGS=""}	# Use an empty default.
: ${CFLAGS=""}		# Don't default to "-g -O2".
: ${CXXFLAGS=""}	# Don't default to "-g -O2".

AC_CANONICAL_BUILD
AM_INIT_AUTOMAKE([
	foreign
	subdir-objects
	color-tests
	std-options
	-Wall
	-Werror
])

# Determine how many CPUs we have on the build machine.
# We use this for building Boost in parallel with multiple jobs.
AX_COUNT_CPUS
AC_SUBST([CPU_COUNT])

# Set up the maintainer compiler flags.
# Increase the default GCC warning level.
AC_SUBST([AM_CPPFLAGS], ["-Wall -Wextra"])
# By default, compile C and C++ with the maximum possible debugging info and least optimization for maximum debugability.
AC_SUBST([AM_CFLAGS], ["-ggdb3 -O3"])
# -Wold-style-cast
AC_SUBST([AM_CXXFLAGS], ["-ggdb3 -O3"])


###
### Checks for programs
###

## Checks for the programs needed to do a build.
AC_PROG_CC
# This is required instead of AC_PROG_CC_C_O by automake's "subdir-objects" option.
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_RANLIB
# @note The following line is to work around an arguable bug in Autoconf 1.12/Libtool.
# (q.v. http://lists.gnu.org/archive/html/automake/2012-05/msg00014.html).
# Probably remove this when/if that is fixed. 
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_SED

## Checks for maintainer tools.

# Autotest needs autom4te, or 'missing's stub for it. 
AM_MISSING_PROG([AUTOM4TE], [autom4te])


###
### Checks for libraries
###

# The Boost source distribution's tarball.
# If the user doesn't already have a suitable Boost installation, we'll do something similar to what we do
# for DParser, and build it from the source tarball and link to it statically.
# The reason here though is more complex:
#   1. Boost (at least as of 1.48.0) wants to install into the wrong /lib directory on some systems, notably some x86_64 systems.
#   2. Neither of the Autoconf macro packages for detecting Boost that I've tried works completely correctly when
#      more than one Boost installation is on a system, e.g. one in /usr/lib and one in /usr/local/lib. E.g., if an
#      old copy is in /usr/include & /usr/lib and a suitable copy is in /usr/local/include and /usr/local/lib, both
#      will by default find the correct paths, but ultimately make the program try to link with the libs in /usr/lib
#      even though it's correctly using the headers in /usr/local/include.
GRVS_ARG_WITH_FILE([FILE_PATH_BOOST_SOURCE_TARBALL], [boost-tarball], [boost source distribution tarball],
	[],[AC_MSG_WARN([No Boost source tarball specified.])])
AM_CONDITIONAL([BUILD_BOOST_FROM_TARBALL],[test -f "$FILE_PATH_BOOST_SOURCE_TARBALL"])

# The list of Boost libraries we need, in a form that Boost's bootstrap.sh can understand.
AC_SUBST([REQUIRED_BOOST_LIBS],[system,thread])
# Extract the "boost_1_xx_x" part of the filename.
BOOST_TARBALL_DIRNAME=$(echo "${FILE_PATH_BOOST_SOURCE_TARBALL}" | grep -o 'boost_._.._.')
BOOST_TARBALL_TARBZ2=$(basename "${FILE_PATH_BOOST_SOURCE_TARBALL}")
BOOST_TARBALL_TAR=$(basename "${BOOST_TARBALL_TARBZ2}" .bz2)
AC_SUBST([BOOST_TARBALL_DIRNAME])
AC_SUBST([BOOST_TARBALL_TARBZ2])
AC_SUBST([BOOST_TARBALL_TAR])


# A path to a boost source tarball must be specified.
# Populate the necessary variables.
AC_SUBST([BOOST_CPPFLAGS],['-I "$(top_builddir)/third_party/$(BOOST_TARBALL_DIRNAME)"'])
AM_CPPFLAGS="$AM_CPPFLAGS $BOOST_CPPFLAGS"
AC_SUBST([BOOST_LDFLAGS],[""])
AC_SUBST([BOOST_LIBTOOL_FLAGS],[""])


###
### Checks for header files.
###

# N/A


###
### Checks for types
###

# N/A


###
### Checks for structures
###

# N/A


###
### Checks for compiler characteristics.
###

AC_LANG_PUSH([C++])
# Determine what switches we need to get at least some level of C++11 support.
CXX11_FLAG=""
for switch in -std=gnu++11 -std=c++11; do
	AC_MSG_CHECKING(whether $CXX supports C++11 features with $switch)
	AX_APPEND_COMPILE_FLAGS([$switch],[CXX11_FLAG])
	AS_IF([test -n $CXX11_FLAG],
		[AC_MSG_RESULT([yes]); break;],
		[AC_MSG_RESULT([no]);])
done
AS_IF([test -n $CXX11_FLAG],
	[AX_APPEND_COMPILE_FLAGS([$CXX11_FLAG],[AM_CXXFLAGS])],
	[AC_MSG_FAILURE([Can't find working C++11 parameter])])

# Check if the C++ compiler supports some flags we'd like to set.
# Append them to AM_CXXFLAGS if it does. 

# Boost generates a ton of these warnings.
AX_APPEND_COMPILE_FLAGS([-Wno-unused-local-typedefs], [AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS([-pthread], [AM_CXXFLAGS])
# To help with profiling.
AX_APPEND_COMPILE_FLAGS([-fno-omit-frame-pointer], [AM_CXXFLAGS])
AC_LANG_POP([C++])


###
### Checks for library functions.
###
AC_SEARCH_LIBS([argp_parse],[argp])

AC_REPLACE_FUNCS([sched_setaffinity])

AC_LANG_PUSH([C++])
# At least gcc 4.8.x claims to support C++11, but its std::regex library is mostly stubbed in.
# Attempt to detect this.  On gcc 4.8.x, std::regex_iterator<> is not defined, which we use anyway, so we'll use that
# type's existence and linkability as our test.
SAVE_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $CXX11_FLAG"
AC_MSG_CHECKING([whether C++ <regex> library is usable])
AC_LINK_IFELSE(
	[AC_LANG_PROGRAM([[#include <regex>]],[[std::regex_iterator<const char *> test;]])],
	[AC_MSG_RESULT([yes])],
	[AC_MSG_ERROR([std::regex_iterator<> is not defined in your C++ standard library. \
gcc < 4.9.0 is known to have this problem.])]
	)
CXXFLAGS="$SAVE_CXXFLAGS"
AC_LANG_POP([C++])


###
### Checks for system services
###

# N/A


###
### Create the Makefiles for the program.
###
AC_CONFIG_FILES([Makefile
	src/Makefile
	third_party/Makefile])
AC_OUTPUT

