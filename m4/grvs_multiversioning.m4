#
# SYNOPSIS
#
#   AXUCG_CHECK_COMPILE_FLAG(BASE-ISA, FLAG, [EXTRA-FLAGS])
#
# DESCRIPTION
#
#   Checks whether the current language's compiler accepts the given FLAG.
#	If it does, AC_SUBST()'s a variable of the form "CXXFLAGS_EXT_[BASE-ISA]_[FLAGNAME]=[FLAG]",
#			where [FLAGNAME] is FLAG with the first two chars removed, and [BASE-ISA] and [FLAGNAME]
#			are transformed by AS_TR_SH() and capitalized.  For instance, a successful test of:
#				AXUCG_CHECK_COMPILE_FLAG([X86_64], [-msse2])
#			results in the AC_SUBST variable:
#				CXXFLAGS_EXT_X86_64_SSE2=-msse2
#	Also on success, AC_SUBST()'s a variable of the form "HAVE_[BASE-ISA]_ISA_EXTENSIONS=true".  This
#	variable is intended to record the fact that at least one flag test from the given base ISA is
#	supported, and may be useful in an Automake conditional.
# 
#	Unconditionally creates an Automake conditional of the form:
#		BUILD_CXXFLAGS_EXT_[BASE-ISA]_[FLAGNAME]
#	This conditional evaluates to true if the test passed, false otherwise.  This is intended for use in
#	conditional compilation of sources based on the compiler's support of the given FLAG.
#
#	EXTRA-FLAGS is optional.  If given, it is added to the current language's default flags when the check is done.
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

#serial 2

# $1 == Base ISA (currently only "X86_64" supported)
# $2 == Compile flag to check.
# $3 == EXTRA-FLAGS
AX_REQUIRE_DEFINED([AX_CHECK_COMPILE_FLAG])
AC_DEFUN([AXUCG_CHECK_COMPILE_FLAG],
[
	AS_VAR_PUSHDEF([ac_var], [axucg_cv_have_$1_$2])[]dnl
	AS_VAR_PUSHDEF([sh_var_postfix], [m4_substr(m4_toupper($2), 2)])[]dnl
	AS_VAR_PUSHDEF([flags_var_name], [CXXFLAGS_EXT_$1_]sh_var_postfix)[]dnl

	AX_CHECK_COMPILE_FLAG([$2],
		[AS_VAR_SET([ac_var], [yes])],
		[AS_VAR_SET([ac_var], [no])],
		[$3])
	
	AS_VAR_IF([ac_var], [yes],
		[
			# Generate a shell var "CXXFLAGS_$1_$2" equal to the given compile flag (i.e. $2).
			# Note: No brackets around "flags_var_name", since it is an Autoconf polymorphic shell variable.
			# Putting it in brackets prevents it from expanding properly. 
			AC_SUBST(flags_var_name, [$2])
							
			# Set a shell var indicating that we have at least one extension to this ISA.
			AC_SUBST([HAVE_$1_ISA_EXTENSIONS],[true])
		],
		[
			# Flag not supported.
			# @todo Should we AC_SUBST() the var name to empty here?
			:
		])
	##AC_RUN_LOG([: flags_var_name="$flags_var_name"])

	# Generate an automake conditional for this flag.
	AM_CONDITIONAL([BUILD_]flags_var_name, [test AS_VAR_GET(ac_var) = yes])
	
	AS_VAR_POPDEF([flags_var_name])
	AS_VAR_POPDEF([sh_var_postfix])
	AS_VAR_POPDEF([ac_var])
])

### @todo
AC_DEFUN([AXUCG_ADD_MULTIVERSIONING_SUPPORT], [
	
	m4_define([x86_64_exts],
		[[-msse2],
		[-msse4.2 -mno-popcnt],
		[-msse4.2 -popcnt]])
	m4_foreach([OPTIONS],
		[x86_64_exts],
		[
			### @todo
		])
])


#
# SYNOPSIS
#
#   AXUCG_COMPILE_IFELSE(body, varname, prologue)
#
# DESCRIPTION
#
#   @todo NEEDS TO GO IN A DIFFERENT FILE, DOCS NEED TO BE WRITTEN.
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

# $1 == body
# $2 == varname
# $3 == prologue
# $4 == action-if-true
# $5 == action-if-false
AC_DEFUN([AXUCG_COMPILE_IFELSE],
[AC_PREREQ(2.64)dnl for _AC_LANG_PREFIX and AS_VAR_IF
AS_VAR_PUSHDEF([CACHEVAR], [axucg_cv_have_type_$2])[]dnl
AS_VAR_PUSHDEF([sh_var_postfix], [m4_toupper($2)])[]dnl
AS_VAR_PUSHDEF([flags_var_name], [HAVE_TYPE_]sh_var_postfix)[]dnl
AC_CACHE_CHECK([whether _AC_LANG library supports $2], CACHEVAR, [
	AC_COMPILE_IFELSE([
		AC_LANG_PROGRAM([$3], [$1])
		],
		[AS_VAR_SET(CACHEVAR, [yes])],
		[AS_VAR_SET(CACHEVAR, [no])])
])
	
	AC_MSG_WARN(sh_var_postfix) #<< As expected
	AC_MSG_WARN([[CACHEVAR] is AS_VAR_GET(CACHEVAR)]) #<< As expected
	AC_MSG_WARN([[flags_var_name] is flags_var_name or AS_VAR_GET(flags_var_name)]) #???
	
AS_VAR_IF(CACHEVAR,yes,
  [m4_default([$4], :)],
  [m4_default([$5], :)])
      
	AS_VAR_POPDEF([flags_var_name])
	AS_VAR_POPDEF([sh_var_postfix])
	AS_VAR_POPDEF([CACHEVAR])
	
])dnl AXUCG_COMPILE_IFELSE

# $1 == types to check for.
# $2 == header.
AC_DEFUN([AXUCG_CHECK_TYPES],
[AC_PREREQ(2.64)
AS_VAR_PUSHDEF([CONF_MACRO_NAME],[HAVE_TYPE_$1])dnl
AC_MSG_WARN([CONF_MACRO_NAME])
AX_CHECK_COMPILE_FLAG([-O2], [AC_DEFINE([CONF_MACRO_NAME], [1], [Define if _AC_LANG std libary supports $1])], [], [],
	[AC_LANG_PROGRAM([$2], [$1 test_var])])
AS_VAR_POPDEF([CONF_MACRO_NAME])
])

