#
# SYNOPSIS
#
#   _AXUCG_COMPILE_IFELSE(body, varname, prologue)
#
# DESCRIPTION
#
#   @todo DOCS NEED TO BE WRITTEN.
#
# LICENSE
#
#   Copyright (c) 2016 Gary R. Van Sickle <grvs@users.sourceforge.net>
#
#   Copying and distribution of this file, with or without modification,
#	are permitted in any medium without royalty provided the copyright
#	notice and this notice are preserved.  This file is offered as-is,
#	without any warranty.
#

######serial 1

# $1 == config var prefix, i.e. "HAVE_TYPE" or "HAVE_FUNC".
# $2 == body
# $3 == varname
# $4 == prologue
# $5 == action-if-true
# $6 == action-if-false
AC_DEFUN([_AXUCG_COMPILE_IFELSE],
[AC_PREREQ(2.64)dnl for _AC_LANG_PREFIX and AS_VAR_IF
dnl Create a cache var name based on the prefix and varname passed in.
AS_VAR_PUSHDEF([CACHEVAR], [axucg_cv_]m4_tolower($1)_[$3])[]dnl
AS_VAR_PUSHDEF([sh_var_postfix], [m4_toupper($3)])[]dnl
AS_VAR_PUSHDEF([flags_var_name], m4_toupper($1)[_]sh_var_postfix)[]dnl
AC_CACHE_CHECK([whether _AC_LANG library supports $3], CACHEVAR, [
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([$4], [$2])
		],
		[AS_VAR_SET(CACHEVAR, [yes])],
		[AS_VAR_SET(CACHEVAR, [no])])
])

dnl @debug	AC_MSG_WARN(sh_var_postfix) #<< As expected
dnl @debug	AC_MSG_WARN([Value of [CACHEVAR] is AS_VAR_GET(CACHEVAR)]) #<< As expected
dnl @debug	AC_MSG_WARN([Value of [flags_var_name] is flags_var_name or AS_VAR_GET(flags_var_name)]) #???

AS_VAR_IF(CACHEVAR,yes,
  [m4_default([$5], [AC_DEFINE([flags_var_name], [1], [Define if function $3 is defined in header $4.])])],
  [m4_default([$6], :)])

	AS_VAR_POPDEF([flags_var_name])
	AS_VAR_POPDEF([sh_var_postfix])
	AS_VAR_POPDEF([CACHEVAR])

])# _AXUCG_COMPILE_IFELSE


# @todo This is broken, should be similar to AXUCG_CHECK_FUNC() below.
# AXUCG_CHECK_TYPES
# $1 == types to check for.
# $2 == header.
AC_DEFUN([AXUCG_CHECK_TYPES],
[AC_PREREQ(2.64)
AS_VAR_PUSHDEF([CONF_MACRO_NAME],[HAVE_TYPE_$1])dnl
AC_MSG_WARN([CONF_MACRO_NAME])
AX_CHECK_COMPILE_FLAG([-O2], [AC_DEFINE([CONF_MACRO_NAME], [1], [Define if _AC_LANG std library supports $1])], [], [],
	[AC_LANG_PROGRAM([$2], [$1 test_var])])
AS_VAR_POPDEF([CONF_MACRO_NAME])
])# AXUCG_CHECK_TYPES


#
# SYNOPSIS
#
# AXUCG_CHECK_FUNC(HEADER, PROG_BODY, FUNCTION_NAME [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
#
AC_DEFUN([AXUCG_CHECK_FUNC],
[AC_PREREQ(2.64)
AS_VAR_PUSHDEF([CONF_MACRO_NAME],[HAVE_FUNC])dnl
_AXUCG_COMPILE_IFELSE(CONF_MACRO_NAME, [$2], [$3], [$1], [$4], [$5])
AS_VAR_POPDEF([CONF_MACRO_NAME])
])# AXUCG_CHECK_FUNC

