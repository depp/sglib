AC_PREREQ(2.61)
AC_INIT([game], [0.1])
AC_CONFIG_SRCDIR([src/base/clock.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_FILES([config.mak])

dnl CHECK_FRAMEWORK(framework, action if found, action if not found)
dnl This will only ever work on Darwin / Mac OS X
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

AC_DEFUN([GET_UNAME], [
  $1=$(uname $2 2>/dev/null) || $1=unknown
  $1="${$1:-unknown}"
])
GET_UNAME([UNAME_S], [-s])
GET_UNAME([UNAME_M], [-m])

AC_PROG_CXX
AC_PROG_CC_C89
AC_PROG_RANLIB
AC_PATH_PROGS([shell], [dash sh bash], [/bin/sh])

PKG_PROG_PKG_CONFIG(0.16)

dnl Check for front-ends.  We check in order of preference, most
dnl preferred first.  By default, we only build the first front-end
dnl that works.
AC_DEFUN([CHECK_FRONTEND], [
  AC_ARG_WITH([$1],
    AS_HELP_STRING([--with-$1], [build $2 front end]),
    [], [
      if test -z "$have_frontend" ; then
        with_$1=auto
      else
        with_$1=no
      fi
    ])
  if test "x$with_$1" != xno ; then
    $3
    if test -n "$AS_TR_CPP([HAVE_$1])" ; then
      if test -n "$have_frontend" ; then
        have_frontend="$have_frontend, $2"
      else
        have_frontend="$2"
      fi
    else
      if test "x$with_$1" != xauto ; then
        AC_MSG_ERROR(
          [--with-$1 was specified but $2 not found])
      fi
    fi
  fi
  AC_SUBST(AS_TR_CPP([HAVE_$1]))
])

dnl Cocoa (Mac OS X)
dnl On Mac OS X, other front-ends may be available (SDL, Gtk, and X11)
dnl but we prefer Cocoa.  On others systems this harmlessly fails.
CHECK_FRONTEND([cocoa], [Cocoa], [
  CHECK_FRAMEWORK([Cocoa], [HAVE_COCOA=yes])
])

dnl Gtk -- note that this requires GtkGLExt
CHECK_FRONTEND([gtk], [Gtk+-2.0 / GtkGLExt], [
  PKG_CHECK_MODULES([GTK], [gtk+-2.0 gtkglext-1.0], [HAVE_GTK=yes], [:])
])

dnl From the changelog: "SDL 1.2.10 is a major release, featuring a
dnl revamp of the build system and many API improvements and bug fixes."
CHECK_FRONTEND([sdl], [LibSDL], [
  AM_PATH_SDL([1.2.10], [HAVE_SDL=yes])
])

dnl Check that we have a front-end enabled
if test -z "$have_frontend" ; then
  AC_MSG_ERROR([no frontends found, try installing development packages])
fi

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
if test -z "$have_image" ; then
  CHECK_FRAMEWORK([ApplicationServices], [
    AC_DEFINE([HAVE_APPLICATIONSERVICES], [1],
              [The Application Services framework is available])
    have_image='Core Graphics framework'
    IMAGE_CFLAGS=-F/System/Library/Frameworks/ApplicationServices.framework/Frameworks
    IMAGE_LIBS="-framework ApplicationServices"
  ])
fi
if test -z "$have_image" ; then
  PKG_CHECK_MODULES([PNG], [libpng], [
    AC_DEFINE([HAVE_LIBPNG], [1], [LibPNG is available])
    have_libpng=yes
  ], [:])
  AC_CHECK_LIB([jpeg], [jpeg_start_decompress], [
    AC_DEFINE([HAVE_LIBJPEG], [1], [LibJPEG is available])
    JPEG_CFLAGS=
    JPEG_LIBS=-ljpeg
    have_libjpeg=yes
  ])
  if test -n "$have_libpng" && test -n "$have_libjpeg" ; then
    have_image='LibPNG, LibJPEG'
    IMAGE_CFLAGS="$PNG_CFLAGS $JPEG_CFLAGS"
    IMAGE_LIBS="$PNG_LIBS $JPEG_LIBS"
  fi
fi
if test -z "$have_image" ; then
  AC_MSG_ERROR([cannot load images, try installing LibPNG and LibJPEG])
fi
AC_SUBST([IMAGE_CFLAGS])
AC_SUBST([IMAGE_LIBS])

dnl Check for some way to render type
if test -z "$have_type" ; then
  PKG_CHECK_MODULES([PANGOCAIRO], [pangocairo], [
    AC_DEFINE([HAVE_PANGOCAIRO], [1], [PangoCairo is available])
    have_type='Pango with Cairo backend'
    TYPE_CFLAGS="$PANGOCAIRO_CFLAGS"
    TYPE_LIBS="$PANGOCAIRO_LIBS"
  ], [
    PKG_CHECK_MODULES([PANGO], [pango], [
      AC_MSG_WARN([Pango is installed but does not have the Cairo backend])
    ], [:])
  ])
fi
if test -z "$have_type" ; then
  AC_MSG_ERROR([cannot render type, try installing Pango])
fi
AC_SUBST([TYPE_CFLAGS])
AC_SUBST([TYPE_LIBS])

dnl Enable warnings with --enable-warnings
AC_ARG_ENABLE(warnings,
  AS_HELP_STRING([--enable-warnings], [enable warnings for GCC]),
  [enable_warnings=$enableval], [enable_warnings=no])
if test "x$enable_warnings" != xno ; then
  CWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes"
  CXXWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare"
  warnings=yes
  if test "x$enable_warnings" == xerror ; then
    CWARN="$CWARN -Werror"
    CXXWARN="$CXXWARN -Werror"
    warnings='yes, as errors'
  fi
else
  warnings='no'
fi
AC_SUBST([CWARN])
AC_SUBST([CXXWARN])

AC_OUTPUT

echo
echo "Front ends:      $have_frontend"
echo "Image loading:   $have_image"
echo "Type rendering:  $have_type"
echo "Warnings:        $warnings"
