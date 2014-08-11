AC_PREREQ(2.61)
AC_INIT([libpce], [0.1], [depp@zdome.net])
AC_CONFIG_SRCDIR([lib/cpu.c])
AC_CONFIG_FILES([Makefile tests/Makefile])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_LIBTOOL

dnl Enable compiler warnings if requested
AC_ARG_ENABLE(warnings,
  AS_HELP_STRING([--enable-warnings], [enable warnings for GCC]),
  [enable_warnings=$enableval], [enable_warnings=no])
if test "x$enable_warnings" != xno ; then
  CWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wwrite-strings -Wmissing-prototypes -Wstrict-prototypes"
  warnings=yes
  if test "x$enable_warnings" = xerror ; then
    CWARN="$CWARN -Werror"
    warnings='yes, as errors'
  fi
else
  warnings='no'
fi
AC_SUBST([CWARN])

AC_OUTPUT
