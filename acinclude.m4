# -*- autoconf -*-

AC_DEFUN([ENABLE_WARNINGS],[
  AC_ARG_ENABLE(warnings,
    [  --enable-warnings       enable warnings for GCC ],
    [enable_warnings=$enableval], [warnings=no])
  if test "x$enable_warnings" != xno ; then
    [CWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes"]
    [CXXWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare"]
    if test "x$enable_warnings" != xerror ; then
      [CWARN="$CWARN -Werror"]
      [CXXWARN="$CXXWARN -Werror"]
    fi
  fi
  AC_SUBST([CWARN])
  AC_SUBST([CXXWARN])
])
