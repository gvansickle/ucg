# configure.ac for UniversalCodeGrep
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

dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([UniversalCodeGrep], [0.2.2], [https://github.com/gvansickle/ucg/issues],,[https://gvansickle.github.io/ucg/])

AC_CONFIG_SRCDIR([src/Globber.cpp])
AC_CONFIG_HEADERS([config.h])
AC_COPYRIGHT([Copyright (C) 2015-2016 Gary R. Van Sickle])

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
# By default, compile C and C++ with the maximum possible debugging info and maximum optimization.
# For debugging, do a "make CXXFLAGS=-O0".
AC_SUBST([AM_CFLAGS], ["-ggdb3 -O3"])
AC_SUBST([AM_CXXFLAGS], ["-ggdb3 -O3"])


###
### Checks for programs
###

## Checks for the programs needed to do a build.
AC_PROG_CC
# This is required instead of AC_PROG_CC_C_O by automake's "subdir-objects" option.
AM_PROG_CC_C_O
AC_PROG_CXXCPP # Needed for the AC_PREPROC_IFELSE() check of pcre version below.
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

# We need libpcre for regex matching.  As of this writing there are no C++11 implementations
# of <regex> in wide circulation (gcc 5.2, clang 3.7) which are robust enough (gcc SIGSEGVs on simple regexs)
# or even exist (clang has no Linux <regex> support) for production use.
#AX_PATH_LIB_PCRE([],
#	[AC_MSG_ERROR([compiling this program requires that libpcre is installed.])])
#AX_CHECK_LIBRARY([PCRE], [pcre.h], [pcre], [],
#                   [AC_MSG_ERROR([Unable to find libpcre])])
AC_SEARCH_LIBS([pcre_compile], [pcre],
					[AC_DEFINE([HAVE_LIBPCRE], [1], [Define if libpcre is available.])],
					[AC_MSG_ERROR([Unable to find libpcre])])
AC_CHECK_FUNCS([pcre_free_study],
	[],
	[AC_MSG_ERROR([Unable to find pcre_free_study.])])
#echo "PCRE_LDFLAGS=${PCRE_LDFLAGS}";
#echo "PCRE_CPPFLAGS=${PCRE_CPPFLAGS}"; 

# Check the version of the PCRE library.  AX_PATH_LIB_PCRE() might find a version older than we need.
AC_LANG_PUSH([C++])
SAVE_CPPFLAGS="$CPPFLAGS";
CPPFLAGS="$CPPFLAGS -P" # -P to prevent #line annotations in gcc >~5.x, which break the sed mechanism below.
AC_MSG_CHECKING([whether libpcre is new enough])
AC_PREPROC_IFELSE(
	[AC_LANG_PROGRAM([[#include <pcre.h>]],[[
		libpcreversion=PCRE_MAJOR . PCRE_MINOR
		]])],
	[libpcreversion=$(cat conftest.i | $SED -n 's/libpcreversion=\(.*\)/\1/p' | tr -d '\t ')],
	[AC_MSG_ERROR([could not determine libpcre version])]
	)
AX_COMPARE_VERSION([$libpcreversion],[ge],[8.21],
	[AC_MSG_RESULT([yes, libpcre version = $libpcreversion])],
	[AC_MSG_ERROR([no, libpcre version = $libpcreversion.  Compiling this program requires a libpcre version >= 8.21])]
	)
CPPFLAGS="$SAVE_CPPFLAGS"
AC_LANG_POP([C++])


###
### Checks for header files.
###

# We don't have this header on MinGW.
AC_CHECK_HEADERS([pwd.h],[],[],[])


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

# Determine what switches we need to get at least some level of C++11 support.
# This macro will replace $CXX with "$CXX -c++11switch".
AX_CXX_COMPILE_STDCXX_11([ext],[mandatory])

AC_LANG_PUSH([C++])
# Check if the C++ compiler supports some flags we'd like to set.
# Append them to AM_CXXFLAGS if it does. 
AX_APPEND_COMPILE_FLAGS([-pthread], [AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS([-Wformat -Wformat-security],[AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS(['-Werror=format-security'],[AM_CXXFLAGS],[-Wformat -Wformat-security])
# Put this back in to help with profiling.
#AX_APPEND_COMPILE_FLAGS([-fno-omit-frame-pointer], [AM_CXXFLAGS])
AC_LANG_POP([C++])


###
### Checks for library functions.
###
AC_SEARCH_LIBS([argp_parse],[argp])

AC_CHECK_FUNCS([sched_setaffinity])

AC_CHECK_FUNCS([get_current_dir_name])

#AC_LANG_PUSH([C++])
# At least gcc 4.8.x claims to support C++11, but its std::regex library is mostly stubbed in.
# Attempt to detect this.  On gcc 4.8.x, std::regex_iterator<> is not defined, which we use anyway, so we'll use that
# type's existence and linkability as our test.
#SAVE_CXXFLAGS="$CXXFLAGS"
#CXXFLAGS="$CXXFLAGS $CXX11_FLAG"
#AC_MSG_CHECKING([whether C++ <regex> library is usable])
#AC_LINK_IFELSE(
#	[AC_LANG_PROGRAM([[#include <regex>]],[[std::regex_iterator<const char *> test;]])],
#	[AC_MSG_RESULT([yes])],
#	[AC_MSG_ERROR([std::regex_iterator<> is not defined in your C++ standard library. \
#gcc < 4.9.0 is known to have this problem.])]
#	)
#CXXFLAGS="$SAVE_CXXFLAGS"
#AC_LANG_POP([C++])


###
### Checks for system services
###

# N/A


###
### Initialize the test suite.
###
# Autoconf-supported test directory.
AC_CONFIG_TESTDIR([tests])
# The Autotest files to be generated by configure.
AC_CONFIG_FILES([tests/Makefile tests/atlocal])


###
### Create the Makefiles for the program.
###
AC_CONFIG_FILES([Makefile
	src/Makefile
	third_party/Makefile])
AC_OUTPUT

