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


# To get access to get_current_dir_name() on Cygwin, we now need to specify _GNU_SOURCE.
# This will do that for us.
AC_USE_SYSTEM_EXTENSIONS

# Determine how many CPUs we have on the build machine.
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
AC_PROG_CXX
## Make sure we get a C++ compiler which supports C++11.
# Determine what switches we need to get at least some level of C++11 support.
# We do this here because this macro will replace $CXX with "$CXX -c++11switch", so we'll use this
# in the AC_LANG_PUSH([C++]) tests below.
# NOTE: The odd-looking ordering here (this before AC_PROG_CXXCPP) is required until the next version
# of this macro gets released.  See http://lists.gnu.org/archive/html/autoconf-archive-maintainers/2016-03/msg00034.html.
AX_CXX_COMPILE_STDCXX_11([ext],[mandatory])
AC_PROG_CXXCPP # Needed for any C++ AC_PREPROC_IFELSE() checks below.
# @note The following line is to work around an arguable bug in Autoconf 1.12/Libtool.
# (q.v. http://lists.gnu.org/archive/html/automake/2012-05/msg00014.html).
# Probably remove this when/if that is fixed. 
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_RANLIB
AC_PROG_SED

# The testsuite needs a way to invoke "script", to fool ucg into thinking it's outputting to a tty.
# Of course, Linux and *BSD (including OSX) "script"'s take different options and parameters, so we
# try to sort it out here.
AC_PATH_PROG([PROG_SCRIPT], [script], [:])
AC_MSG_CHECKING([$PROG_SCRIPT's command line type])
AS_IF([$PROG_SCRIPT -qfc "echo hello" /dev/null > /dev/null 2>&1],
		[
		AC_SUBST([PROG_SCRIPT_TYPE], [linux])
		AC_SUBST([PROG_SCRIPT_PRE_TEXT], ['-qfc'])
		AC_SUBST([PROG_SCRIPT_POST_TEXT], ['/dev/null'])
		AC_MSG_RESULT([Linux style])
		],
      [$PROG_SCRIPT -q /dev/null echo "hello" > /dev/null 2>&1],
		[
		AC_SUBST([PROG_SCRIPT_TYPE], [bsd])
		AC_SUBST([PROG_SCRIPT_PRE_TEXT], ['-q /dev/null'])
		AC_SUBST([PROG_SCRIPT_POST_TEXT], [])
		AC_MSG_RESULT([BSD style])
		],
      [AC_MSG_ERROR([could not determine how to invoke $PROG_SCRIPT])])

## Checks for maintainer tools.

# Autotest needs autom4te, or 'missing's stub for it. 
AM_MISSING_PROG([AUTOM4TE], [autom4te])


###
### Checks for libraries
###

AC_LANG_PUSH([C++])

# We need libpcre for regex matching.  As of this writing there are no C++11 implementations
# of <regex> in wide circulation (gcc 5.2, clang 3.7) which are robust enough (gcc SIGSEGVs on simple regexs)
# or even exist (clang has no Linux <regex> support) for production use.
PKG_CHECK_MODULES([PCRE], [libpcre >= 8.21],
	[AC_DEFINE([HAVE_LIBPCRE], [1], [Define if libpcre is available.])],
	[AC_MSG_ERROR([Unable to find libpcre])])
AC_MSG_NOTICE([PCRE_LIBS=${PCRE_LIBS}])
AC_MSG_NOTICE([PCRE_CFLAGS=${PCRE_CFLAGS}])
CPPFLAGS="$PCRE_CFLAGS $CPPFLAGS"
LIBS="$PCRE_LIBS $LIBS"

PKG_CHECK_MODULES([PCRE2], [libpcre2-8 >= 10.0],
	[AC_DEFINE([HAVE_LIBPCRE2], [1], [Define if libpcre2 is available.])],
	[AC_MSG_WARN([Unable to find libpcre2])])
CPPFLAGS="$PCRE2_CFLAGS $CPPFLAGS"
LIBS="$PCRE2_LIBS $LIBS"

AC_LANG_POP([C++])


###
### Checks for header files.
###

AC_LANG_PUSH([C++])

# We don't have this header on MinGW.
AC_CHECK_HEADERS([pwd.h],[],[],[])

AC_LANG_POP([C++])

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
# Check if the C++ compiler supports some flags we'd like to set.
# Append them to AM_CXXFLAGS if it does.

# Capture the compile command line in the binary.
AX_APPEND_COMPILE_FLAGS([-frecord-gcc-switches], [AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS([-Wnoexcept], [AM_CXXFLAGS]) # Warn if something isn't marked noexcept when it could be.
AX_APPEND_COMPILE_FLAGS([-ftree-vectorize], [AM_CXXFLAGS]) #  -fopt-info-vec-all
#AX_APPEND_COMPILE_FLAGS([-flto -ffat-lto-objects], [AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS([-pthread], [AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS([-Wformat -Wformat-security],[AM_CXXFLAGS])
AX_APPEND_COMPILE_FLAGS(['-Werror=format-security'],[AM_CXXFLAGS],[-Wformat -Wformat-security])

# Put this back in to help with profiling.
#AX_APPEND_COMPILE_FLAGS([-fno-omit-frame-pointer], [AM_CXXFLAGS])

AC_LANG_POP([C++])


###
### Checks for library functions.
###

AC_LANG_PUSH([C++])

AC_SEARCH_LIBS([argp_parse],[argp])

AC_CHECK_FUNCS([sched_setaffinity])

AC_CHECK_FUNCS([get_current_dir_name])

AC_LANG_POP([C++])

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

