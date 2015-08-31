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
#
# SYNOPSIS
#
#   GRVS_ARG_WITH_DIR([VARIABLE], [with-option-suffix], [description of the directory],
#      [ACTION_IF_FOUND], [ACTION_IF_NOT_FOUND])
#
# DESCRIPTION
#
#   Adds options to the generated configure which requre a directory to be specified with either:
#       "configure --with-<with-option-suffix>=/path/to/dir"
#   or:
#       "configure VARIABLE=/path/to/file"
#   Specified file is checked for existence, readability, and that it's a regular directory.  Configuration
#   aborts if the file is not specified or if any of the checks fail.
#   The given path is stored in the precious variable VARIABLE.
#

# SYNOPSIS
#
#   GRVS_ARG_WITH_FILE([VARIABLE], [with-option-suffix], [description of the file],
#      [ACTION_IF_FOUND], [ACTION_IF_NOT_FOUND])
#
# DESCRIPTION
#
#   Adds options to the generated configure which requre a file to be specified with either:
#       "configure --with-<with-option-suffix>=/path/to/file"
#   or:
#       "configure VARIABLE=/path/to/file"
#   Specified file is checked for existence, readability, and that it's a regular file.  Configuration
#   aborts if the file is not specified or if any of the checks fail.
#   The given path is stored in the precious variable VARIABLE.
# 

#serial 3

# The common part of the GRVS_ARG_WITH_xxx macros.
AC_DEFUN([_GRVS_ARG_WITH_SOMETHING],
[
	AC_PREREQ([2.68])

	m4_pushdef([VARIABLE],$1)
	m4_pushdef([ARG_TEXT],$2)
	m4_pushdef([VAR_AND_OPT_DESCRIPTION],$3)
	m4_pushdef([ACTION_IF_FOUND],$4)
	m4_pushdef([ACTION_IF_NOT_FOUND],[m4_default([$5],
		[
			# Default ACTION_IF_NOT_FOUND is to abort with an error message.
			AC_MSG_FAILURE([Either --with-ARG_TEXT=PATH or VARIABLE=PATH must be specified.])
		])
	])
	m4_pushdef([TEST_D_OR_F],$6)	
	
	# Temp vars.
	grvs_exit_no_path_specified=no
	
	# Add the precious variable and associated help text.
	AC_ARG_VAR(VARIABLE,Absolute path to VAR_AND_OPT_DESCRIPTION.)
	
	# Check if the associated variable is already defined.
	AS_IF([test -z "$VARIABLE"],
	[
		# It isn't.
		AC_MSG_CHECKING([if VAR_AND_OPT_DESCRIPTION was specified])
		AC_ARG_WITH(ARG_TEXT,AS_HELP_STRING([--with-ARG_TEXT=PATH],[Absolute path to VAR_AND_OPT_DESCRIPTION.]),
		[
			# The option was passed, but check if the user didn't provide a value for PATH,
			# or gave a "yes" or "no" instead of a path. 
			AS_IF([test "$withval" != yes && test "$withval" != no && test -n "$withval"],
			[
				# The provided path was something other than empty, "yes", or "no".
				VARIABLE="$withval"
				AC_MSG_RESULT([yes: "$VARIABLE"])
			],
			[
				# Nothing that might be a path was provided.
				VARIABLE=""
			])
		])
		
		# See if we now have a defined and non-empty VARIABLE.
		AS_IF([test -z "$VARIABLE"],
		[
			# If VARIABLE is still empty at this point, we didn't get it from either AC_ARG_VAR or AC_ARG_WITH,
			# so we have to report that we can't find it.
			AC_MSG_RESULT([no])
			ACTION_IF_NOT_FOUND
			grvs_exit_no_path_specified=yes
		])
	])
	
	if test "x$grvs_exit_no_path_specified" != "xyes"; then
		# If we haven't failed yet, make sure the path we return is an absolute path.
		VARIABLE=`readlink -fn "$VARIABLE"`		
		
		# Check if the specified file exists and is a regular file.
		AS_CASE(["TEST_D_OR_F"],
			["d"], [MESSAGE_TEXT_FILE_OR_DIR="directory"],
			["f"], [MESSAGE_TEXT_FILE_OR_DIR="file"],
			[
				AC_MSG_FAILURE([INTERNAL ERROR: Neither "d" nor "f" was specified])
			])
		AC_MSG_CHECKING([if specified VAR_AND_OPT_DESCRIPTION \"$VARIABLE\" exists and is a regular $MESSAGE_TEXT_FILE_OR_DIR])
		AS_IF([test -TEST_D_OR_F "$VARIABLE"],
		[
			AC_MSG_RESULT([yes])
		],
		[
			AC_MSG_RESULT([no])
			AC_MSG_FAILURE([path does not exist or is not a regular $MESSAGE_TEXT_FILE_OR_DIR: "$VARIABLE"])
		])
		
		# Check if the specified dir is readable by us.
		AC_MSG_CHECKING([if specified VAR_AND_OPT_DESCRIPTION \"$VARIABLE\" is readable])
		AS_IF([test -r "$VARIABLE"],
		[
			AC_MSG_RESULT([yes])
		],
		[
			AC_MSG_RESULT([no])
			AC_MSG_FAILURE([file is not readable: $VARIABLE])
		])
	fi
	
	m4_popdef([TEST_D_OR_F])	
	m4_popdef([ACTION_IF_NOT_FOUND])
	m4_popdef([ACTION_IF_FOUND])
	m4_popdef([VAR_AND_OPT_DESCRIPTION])
	m4_popdef([ARG_TEXT])
	m4_popdef([VARIABLE])
])# _GRVS_ARG_WITH_SOMETHING


AC_DEFUN([GRVS_ARG_WITH_DIR],
[
	_GRVS_ARG_WITH_SOMETHING([$1],[$2],[$3],[$4],[$5],[d])
])# GRVS_ARG_WITH_DIR

AC_DEFUN([GRVS_ARG_WITH_FILE],
[
	_GRVS_ARG_WITH_SOMETHING([$1],[$2],[$3],[$4],[$5],[f])
])# GRVS_ARG_WITH_FILE

