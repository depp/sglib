# -*- autoconf -*-

AC_DEFUN([ENABLE_WARNINGS],[
  AC_ARG_ENABLE(warnings,
    [  --enable-warnings       enable warnings for GCC ],
    [enable_warnings=$enableval], [warnings=no])
  if test "x$enable_warnings" != xno ; then
    [CWARN="-Wall -Wextra -Wno-sign-compare -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes"]
    [CXXWARN="-Wall -Wextra -Wno-sign-compare"]
    if test "x$enable_warnings" != xyes ; then
      [CWARN="$CWARN $enable_warnings"]
      [CXXWARN="$CXXWARN $enable_warnings"]
    fi
  fi
  AC_SUBST([CWARN])
  AC_SUBST([CXXWARN])
])
