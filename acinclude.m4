# -*- autoconf -*-

AC_DEFUN([ENABLE_WARNINGS],[
  AC_ARG_ENABLE(warnings,
    [  --enable-warnings       enable warnings for GCC ],
    [enable_warnings=$enableval], [warnings=no])
  if test "x$enable_warnings" != xno ; then
    [warning_cppflags="-Wall -Wextra -Wno-unused-parameter -Wno-sign-compare"]
    if test "x$enable_warnings" != xyes ; then
      [warning_cppflags="$warning_cppflags $enable_warnings"]
    fi
  fi
  AC_SUBST([warning_cppflags])
])
