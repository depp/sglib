AC_PREREQ(2.61)
AC_INIT([game], [0.1])
AC_CONFIG_SRCDIR([src/base/clock.c])
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

dnl CHECK_FRAMEWORK(framework, action if found, action if not found)
AC_DEFUN([CHECK_FRAMEWORK], [
  AC_CACHE_CHECK([for the $1 framework], [my_cv_framework_$1], [
    save_LIBS="$LIBS"
    LIBS="$LIBS -framework $1"
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([], [])],
      [my_cv_framework_$1=yes],
      [my_cv_framework_$1=no])
  ])
  LIBS="$save_LIBS"
  if test "$my_framework_$1" = yes
  then
    m4_default([$2], [:])
  else
    m4_default([$3], [:])
  fi
])

dnl AC_DEFUN([GET_UNAME], [
dnl   $1=$(uname $2 2>/dev/null) || $1=unknown
dnl   if test -z "$1" ; then $1=unknown ; fi
dnl ])
dnl GET_UNAME([UNAME_S], [-s])
dnl GET_UNAME([UNAME_M], [-m])

AC_PROG_CXX
AC_PROG_CC_C89
AC_PROG_RANLIB
AC_PATH_PROGS([shell], [dash sh bash], [/bin/sh])

PKG_PROG_PKG_CONFIG(0.16)

dnl Check for OpenGL
if test -z "$have_opengl" ; then
  CHECK_FRAMEWORK([OpenGL], [
    have_opengl=yes
    OPENGL_CFLAGS=
    OPENGL_LIBS="-framework OpenGL"
  ])
fi
if test -z "$have_opengl" ; then
  AC_CHECK_LIB([GL], [glClear], [
    AC_CHECK_LIB([GLU], [gluErrorString], [
      have_opengl=yes
      OPENGL_CFLAGS=
      OPENGL_LIBS="-lGL -lGLU"
    ])
  ])
fi
if test -z "$have_opengl" ; then
  AC_MSG_ERROR([cannot find OpenGL libraries])
fi
AC_SUBST([GL_LIBS])
AC_SUBST([GL_CFLAGS])

dnl Check for some way to load images
if test -z "$have_images" ; then
  CHECK_FRAMEWORK([ApplicationServices], [
    AC_DEFINE([HAVE_APPLICATIONSERVICES], [1],
              [The Application Services framework is available])
    have_images='Core Graphics framework'
    IMAGE_CFLAGS=-F/System/Library/Frameworks/ApplicationServices.framework/Frameworks
    IMAGE_LIBS="-framework ApplicationServices"
  ])
fi
if test -z "$have_images" ; then
  PKG_CHECK_MODULES([PNG], [libpng], [
    AC_DEFINE([HAVE_LIBPNG], [1], [LibPNG is available])
    have_libpng=yes
  ])
  AC_CHECK_LIB([jpeg], [jpeg_start_decompress], [
    AC_DEFINE([HAVE_LIBJPEG], [1], [LibJPEG is available])
    JPEG_CFLAGS=
    JPEG_LIBS=-ljpeg
    have_libjpeg=yes
  ])
  if test -n "$have_libpng" && test -n "$have_libjpeg" ; then
    have_images='LibPNG and LibJPEG'
    IMAGE_CFLAGS="$PNG_CFLAGS $JPEG_CFLAGS"
    IMAGE_LIBS="$PNG_LIBS $JPEG_LIBS"
  fi
fi
if test -z "$have_images" ; then
  AC_MSG_ERROR([cannot load images, try installing LibPNG and LibJPEG])
fi
AC_SUBST([IMAGE_CFLAGS])
AC_SUBST([IMAGE_LIBS])

dnl Check for some way to render type
if test -z "$have_type" ; then
  PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo], [
    AC_DEFINE([HAVE_PANGOCAIRO], [1], [PangoCairo is available])
    have_type='Pango'
    TYPE_CFLAGS="$PANGOCAIRO_CFLAGS"
    TYPE_LIBS="$PANGOCAIRO_LIBS"
  ], [
    PKG_CHECK_MODULES([PANGO], [pango], [
      AC_MSG_WARN([Pango is installed but does not have the Cairo backend])
    ])
  ])
fi
if test -z "$have_type" ; then
  AC_MSG_ERROR([cannot render type, try installing Pango])
fi
AC_SUBST([TYPE_CFLAGS])
AC_SUBST([TYPE_LIBS])

dnl Check for front-ends
PKG_CHECK_MODULES([GTK], [gtk+-2.0 gtkglext-1.0], [
  HAVE_GTK=yes
  AC_SUBST([HAVE_GTK])
  frontends="Gtk $frontends"
])
dnl From the changelog: "SDL 1.2.10 is a major release, featuring a
dnl revamp of the build system and many API improvements and bug fixes."
AM_PATH_SDL([1.2.10], [
  HAVE_SDL=yes
  AC_SUBST([HAVE_SDL])
  frontends="SDL $frontends"
])
if test -z "$frontends" ; then
  AC_MSG_ERROR([no frontends found, try installing development packages])
fi

dnl Enable warnings with --enable-warnings
ENABLE_WARNINGS

AC_OUTPUT
