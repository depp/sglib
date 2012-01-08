AC_PREREQ(2.61)
AC_INIT([game], [0.1])
AC_CONFIG_SRCDIR([src/gtk/main.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([config.mak])


AC_DEFUN([ENABLE_WARNINGS],[
  AC_ARG_ENABLE(warnings,
    [  --enable-warnings       enable warnings for GCC ],
    [enable_warnings=$enableval], [enable_warnings=no])
  if test "x$enable_warnings" != xno ; then
    [CWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes"]
    [CXXWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare"]
    if test "x$enable_warnings" == xerror ; then
      [CWARN="$CWARN -Werror"]
      [CXXWARN="$CXXWARN -Werror"]
    fi
  fi
  AC_SUBST([CWARN])
  AC_SUBST([CXXWARN])
])

echo WARNINGS $enable_warnings


UNAME_S=$(uname -s 2>/dev/null || echo unknown)
UNAME_M=$(uname -m 2>/dev/null || echo unknown)


AC_PROG_CXX
AC_PROG_CC_C89
AC_PROG_RANLIB
AC_PATH_PROGS([shell], [dash sh bash], [/bin/sh])

dnl # From the changelog: "SDL 1.2.10 is a major release, featuring a
dnl # revamp of the build system and many API improvements and bug fixes."
dnl AM_PATH_SDL(
dnl   [1.2.10],
dnl   [],
dnl   AC_MSG_ERROR([LibSDL 1.2.10 or greater is required]))

dnl Fix this later
GL_LIBS="-lGL -lGLU"
AC_SUBST([GL_LIBS])
AC_SUBST([GL_CFLAGS])

PKG_PROG_PKG_CONFIG(0.16)

PKG_CHECK_MODULES(
  [PNG], [libpng],
  [AC_DEFINE([HAVE_LIBPNG], [1], [LibPNG is available])])
AC_CHECK_LIB(
  [jpeg], [jpeg_start_decompress],
  [AC_DEFINE([HAVE_LIBJPEG], [1], [LibJPEG is available])
   JPEG_LDFLAGS=-ljpeg])
AC_SUBST([JPEG_CFLAGS])
AC_SUBST([JPEG_LDFLAGS])
PKG_CHECK_MODULES([GTK], [gtk+-2.0 gtkglext-1.0])

ENABLE_WARNINGS

AC_OUTPUT
