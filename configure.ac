AC_PREREQ(2.61)
AC_INIT([game], [0.1])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/gtk/main.cpp])
AC_CONFIG_FILES([Makefile src/Makefile])

AC_DEFINE(_GNU_SOURCE)
AC_PROG_CXX
AC_PROG_RANLIB

dnl # From the changelog: "SDL 1.2.10 is a major release, featuring a
dnl # revamp of the build system and many API improvements and bug fixes."
dnl AM_PATH_SDL(
dnl   [1.2.10],
dnl   [],
dnl   AC_MSG_ERROR([LibSDL 1.2.10 or greater is required]))

AC_SEARCH_LIBS([glVertex3f], [GL])
AC_SEARCH_LIBS([gluErrorString], [GLU])
PKG_PROG_PKG_CONFIG(0.16)
PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo])
PKG_CHECK_MODULES([GTK], [gtk+-2.0])
PKG_CHECK_MODULES([GTKGLEXT], [gtkglext-1.0])
CHECK_RAGEL_AM([../src/config/configfileread.cpp])

ENABLE_WARNINGS

AC_OUTPUT
