# -*- autoconf -*-

AC_DEFUN([ENABLE_WARNINGS],[
  AC_ARG_ENABLE(warnings,
    [  --enable-warnings       enable warnings for GCC ],
    [enable_warnings=$enableval], [warnings=no])
  if test "x$enable_warnings" != xno ; then
    [warning_cflags="-Wall -Wextra -Wno-unused-parameter -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs -Wchar-subscripts -Wredundant-decls -pedantic"]
    if test "x$enable_warnings" != xyes ; then
      [warning_cflags="$warning_cflags $enable_warnings"]
    fi
  fi
  AC_SUBST([warning_cflags])
])
