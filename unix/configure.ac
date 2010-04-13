AC_PREREQ(2.61)
AC_INIT([game], [0.1])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([../src/main.cpp])
AC_CONFIG_FILES([Makefile])

AC_DEFINE(_GNU_SOURCE)
AC_PROG_CXX

# From the changelog: "SDL 1.2.10 is a major release, featuring a
# revamp of the build system and many API improvements and bug fixes."
AM_PATH_SDL(
  [1.2.10],
  [],
  AC_MSG_ERROR([LibSDL 1.2.10 or greater is required]))

AC_SEARCH_LIBS([glVertex3f], [GL])

ENABLE_WARNINGS

AC_OUTPUT
