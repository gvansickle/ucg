# Process this file with autoconf to produce a configure script.
AC_INIT([dummy], [0])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([glm4])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# For autobuild.
AC_CANONICAL_BUILD
AC_CANONICAL_HOST

m4_pattern_forbid([^gl_[A-Z]])dnl the gnulib macro namespace
m4_pattern_allow([^gl_ES$])dnl a valid locale name
m4_pattern_allow([^gl_LIBOBJS$])dnl a variable
m4_pattern_allow([^gl_LTLIBOBJS$])dnl a variable

# Pre-early section.
gl_USE_SYSTEM_EXTENSIONS
gl_PROG_AR_RANLIB

LT_INIT([win32-dll])
LT_LANG([C++])
AM_CONDITIONAL([GL_COND_LIBTOOL], [true])
gl_cond_libtool=true
AC_DEFUN([gl_INIT], [
gl_m4_base='glm4'
  m4_pushdef([AC_LIBOBJ], m4_defn([gl_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gl_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gl_LIBSOURCES]))
  m4_pushdef([gl_LIBSOURCES_LIST], [])
  m4_pushdef([gl_LIBSOURCES_DIR], [])
  gl_COMMON
gl_source_base='gllib'
changequote(,)dnl
LTALLOCA=`echo "$ALLOCA" | sed -e 's/\.[^.]* /.lo /g;s/\.[^.]*$/.lo/'`
changequote([, ])dnl
AC_SUBST([LTALLOCA])
  gl_FUNC_ALLOCA
  gl_ARGP
  m4_ifdef([AM_XGETTEXT_OPTION],
    [AM_][XGETTEXT_OPTION([--flag=argp_error:2:c-format])
     AM_][XGETTEXT_OPTION([--flag=argp_failure:4:c-format])])
  gl_DIRNAME_LGPL
  gl_DOUBLE_SLASH_ROOT
  gl_HEADER_ERRNO_H
  AC_REQUIRE([gl_EXTERN_INLINE])
  gl_FLOAT_H
  if test $REPLACE_FLOAT_LDBL = 1; then
    AC_LIBOBJ([float])
  fi
  if test $REPLACE_ITOLD = 1; then
    AC_LIBOBJ([itold])
  fi
  gl_FUNC_GETOPT_GNU
  if test $REPLACE_GETOPT = 1; then
    AC_LIBOBJ([getopt])
    AC_LIBOBJ([getopt1])
    gl_PREREQ_GETOPT
    dnl Arrange for unistd.h to include getopt.h.
    GNULIB_GL_UNISTD_H_GETOPT=1
  fi
  AC_SUBST([GNULIB_GL_UNISTD_H_GETOPT])
  gl_MODULE_INDICATOR_FOR_TESTS([getopt-gnu])
  gl_FUNC_GETOPT_POSIX
  if test $REPLACE_GETOPT = 1; then
    AC_LIBOBJ([getopt])
    AC_LIBOBJ([getopt1])
    gl_PREREQ_GETOPT
    dnl Arrange for unistd.h to include getopt.h.
    GNULIB_GL_UNISTD_H_GETOPT=1
  fi
  AC_SUBST([GNULIB_GL_UNISTD_H_GETOPT])
  gl_LIMITS_H
  gl_FUNC_MALLOC_GNU
  if test $REPLACE_MALLOC = 1; then
    AC_LIBOBJ([malloc])
  fi
  gl_MODULE_INDICATOR([malloc-gnu])
  gl_FUNC_MALLOC_POSIX
  if test $REPLACE_MALLOC = 1; then
    AC_LIBOBJ([malloc])
  fi
  gl_STDLIB_MODULE_INDICATOR([malloc-posix])
  gl_FUNC_MEMCHR
  if test $HAVE_MEMCHR = 0 || test $REPLACE_MEMCHR = 1; then
    AC_LIBOBJ([memchr])
    gl_PREREQ_MEMCHR
  fi
  gl_STRING_MODULE_INDICATOR([memchr])
  gl_FUNC_MEMPCPY
  if test $HAVE_MEMPCPY = 0; then
    AC_LIBOBJ([mempcpy])
    gl_PREREQ_MEMPCPY
  fi
  gl_STRING_MODULE_INDICATOR([mempcpy])
  gl_MULTIARCH
  gl_FUNC_SLEEP
  if test $HAVE_SLEEP = 0 || test $REPLACE_SLEEP = 1; then
    AC_LIBOBJ([sleep])
  fi
  gl_UNISTD_MODULE_INDICATOR([sleep])
  gt_TYPE_SSIZE_T
  gl_STDALIGN_H
  AM_STDBOOL_H
  gl_STDDEF_H
  gl_STDINT_H
  gl_STDIO_H
  gl_STDLIB_H
  gl_STRCASE
  if test $HAVE_STRCASECMP = 0; then
    AC_LIBOBJ([strcasecmp])
    gl_PREREQ_STRCASECMP
  fi
  if test $HAVE_STRNCASECMP = 0; then
    AC_LIBOBJ([strncasecmp])
    gl_PREREQ_STRNCASECMP
  fi
  gl_FUNC_STRCHRNUL
  if test $HAVE_STRCHRNUL = 0 || test $REPLACE_STRCHRNUL = 1; then
    AC_LIBOBJ([strchrnul])
    gl_PREREQ_STRCHRNUL
  fi
  gl_STRING_MODULE_INDICATOR([strchrnul])
  gl_FUNC_STRERROR
  if test $REPLACE_STRERROR = 1; then
    AC_LIBOBJ([strerror])
  fi
  gl_MODULE_INDICATOR([strerror])
  gl_STRING_MODULE_INDICATOR([strerror])
  gl_HEADER_STRING_H
  gl_HEADER_STRINGS_H
  gl_FUNC_STRNDUP
  if test $HAVE_STRNDUP = 0 || test $REPLACE_STRNDUP = 1; then
    AC_LIBOBJ([strndup])
  fi
  gl_STRING_MODULE_INDICATOR([strndup])
  gl_SYS_TYPES_H
  AC_PROG_MKDIR_P
  gl_SYSEXITS
  gl_UNISTD_H
  gl_FUNC_VSNPRINTF
  gl_STDIO_MODULE_INDICATOR([vsnprintf])
  gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36=false
  gl_gnulib_enabled_intprops=false
  gl_gnulib_enabled_rawmemchr=false
  gl_gnulib_enabled_size_max=false
  gl_gnulib_enabled_dbb57f49352be8fb86869629a254fb72=false
  gl_gnulib_enabled_strnlen=false
  gl_gnulib_enabled_vasnprintf=false
  gl_gnulib_enabled_verify=false
  gl_gnulib_enabled_wchar=false
  gl_gnulib_enabled_xsize=false
  func_gl_gnulib_m4code_be453cec5eecf5731a274f2de7f2db36 ()
  {
    if ! $gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36; then
      AC_SUBST([LIBINTL])
      AC_SUBST([LTLIBINTL])
      gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36=true
    fi
  }
  func_gl_gnulib_m4code_intprops ()
  {
    if ! $gl_gnulib_enabled_intprops; then
      gl_gnulib_enabled_intprops=true
      func_gl_gnulib_m4code_verify
    fi
  }
  func_gl_gnulib_m4code_rawmemchr ()
  {
    if ! $gl_gnulib_enabled_rawmemchr; then
      gl_FUNC_RAWMEMCHR
      if test $HAVE_RAWMEMCHR = 0; then
        AC_LIBOBJ([rawmemchr])
        gl_PREREQ_RAWMEMCHR
      fi
      gl_STRING_MODULE_INDICATOR([rawmemchr])
      gl_gnulib_enabled_rawmemchr=true
    fi
  }
  func_gl_gnulib_m4code_size_max ()
  {
    if ! $gl_gnulib_enabled_size_max; then
      gl_SIZE_MAX
      gl_gnulib_enabled_size_max=true
    fi
  }
  func_gl_gnulib_m4code_dbb57f49352be8fb86869629a254fb72 ()
  {
    if ! $gl_gnulib_enabled_dbb57f49352be8fb86869629a254fb72; then
      AC_REQUIRE([gl_HEADER_ERRNO_H])
      AC_REQUIRE([gl_FUNC_STRERROR_0])
      if test -n "$ERRNO_H" || test $REPLACE_STRERROR_0 = 1; then
        AC_LIBOBJ([strerror-override])
        gl_PREREQ_SYS_H_WINSOCK2
      fi
      gl_gnulib_enabled_dbb57f49352be8fb86869629a254fb72=true
    fi
  }
  func_gl_gnulib_m4code_strnlen ()
  {
    if ! $gl_gnulib_enabled_strnlen; then
      gl_FUNC_STRNLEN
      if test $HAVE_DECL_STRNLEN = 0 || test $REPLACE_STRNLEN = 1; then
        AC_LIBOBJ([strnlen])
        gl_PREREQ_STRNLEN
      fi
      gl_STRING_MODULE_INDICATOR([strnlen])
      gl_gnulib_enabled_strnlen=true
    fi
  }
  func_gl_gnulib_m4code_vasnprintf ()
  {
    if ! $gl_gnulib_enabled_vasnprintf; then
      gl_FUNC_VASNPRINTF
      gl_gnulib_enabled_vasnprintf=true
      func_gl_gnulib_m4code_verify
      func_gl_gnulib_m4code_wchar
      func_gl_gnulib_m4code_xsize
    fi
  }
  func_gl_gnulib_m4code_verify ()
  {
    if ! $gl_gnulib_enabled_verify; then
      gl_gnulib_enabled_verify=true
    fi
  }
  func_gl_gnulib_m4code_wchar ()
  {
    if ! $gl_gnulib_enabled_wchar; then
      gl_WCHAR_H
      gl_gnulib_enabled_wchar=true
    fi
  }
  func_gl_gnulib_m4code_xsize ()
  {
    if ! $gl_gnulib_enabled_xsize; then
      gl_XSIZE
      gl_gnulib_enabled_xsize=true
      func_gl_gnulib_m4code_size_max
    fi
  }
  if test $REPLACE_GETOPT = 1; then
    func_gl_gnulib_m4code_be453cec5eecf5731a274f2de7f2db36
  fi
  if test $HAVE_SLEEP = 0 || test $REPLACE_SLEEP = 1; then
    func_gl_gnulib_m4code_verify
  fi
  if test $HAVE_STRCHRNUL = 0 || test $REPLACE_STRCHRNUL = 1; then
    func_gl_gnulib_m4code_rawmemchr
  fi
  if test $REPLACE_STRERROR = 1; then
    func_gl_gnulib_m4code_intprops
  fi
  if test $REPLACE_STRERROR = 1; then
    func_gl_gnulib_m4code_dbb57f49352be8fb86869629a254fb72
  fi
  if test $REPLACE_STRERROR = 1; then
    func_gl_gnulib_m4code_verify
  fi
  if test $HAVE_STRNDUP = 0 || test $REPLACE_STRNDUP = 1; then
    func_gl_gnulib_m4code_strnlen
  fi
  if test $ac_cv_func_vsnprintf = no || test $REPLACE_VSNPRINTF = 1; then
    func_gl_gnulib_m4code_vasnprintf
  fi
  m4_pattern_allow([^gl_GNULIB_ENABLED_])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_be453cec5eecf5731a274f2de7f2db36], [$gl_gnulib_enabled_be453cec5eecf5731a274f2de7f2db36])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_intprops], [$gl_gnulib_enabled_intprops])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_rawmemchr], [$gl_gnulib_enabled_rawmemchr])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_size_max], [$gl_gnulib_enabled_size_max])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_dbb57f49352be8fb86869629a254fb72], [$gl_gnulib_enabled_dbb57f49352be8fb86869629a254fb72])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_strnlen], [$gl_gnulib_enabled_strnlen])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_vasnprintf], [$gl_gnulib_enabled_vasnprintf])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_verify], [$gl_gnulib_enabled_verify])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_wchar], [$gl_gnulib_enabled_wchar])
  AM_CONDITIONAL([gl_GNULIB_ENABLED_xsize], [$gl_gnulib_enabled_xsize])
  m4_ifval(gl_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gl_LIBSOURCES_DIR])[ ||
      for gl_file in ]gl_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([gl_LIBSOURCES_DIR])
  m4_popdef([gl_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gl_libobjs=
    gl_ltlibobjs=
    if test -n "$gl_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gl_LIBOBJS; do echo "$i"; done | sed -e "$sed_drop_objext" | sort | uniq`; do
        gl_libobjs="$gl_libobjs $i.$ac_objext"
        gl_ltlibobjs="$gl_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gl_LIBOBJS], [$gl_libobjs])
    AC_SUBST([gl_LTLIBOBJS], [$gl_ltlibobjs])
  ])
])

# Like AC_LIBOBJ, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_LIBOBJ], [
  AS_LITERAL_IF([$1], [gl_LIBSOURCES([$1.c])])dnl
  gl_LIBOBJS="$gl_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gl_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gl_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gl_LIBSOURCES_DIR], [gllib])
      m4_append([gl_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

gl_INIT

AC_CONFIG_FILES([Makefile gllib/Makefile glm4/Makefile])
AC_OUTPUT
