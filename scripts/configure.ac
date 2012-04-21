AC_PREREQ(2.61)
AC_INIT([@PKG_NAME@], [@PKG_APP_VERSION@], [@PKG_EMAIL@],
        [@PKG_FILENAME@], [@PKG_URL@])
AC_CONFIG_SRCDIR([@SRCFILE@])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([config.mak])

dnl Check if the linker supports the given flags
dnl Don't call directly, use for implementing macros
dnl CHECK_LDFLAGS(flag, check description,
dnl               action if found, action if not found)
AC_DEFUN([CHECK_LDFLAGS_BASE], [
  AS_VAR_PUSHDEF([cachevar], [my_cv_ldflag_$1])
  AC_CACHE_CHECK([$2], cachevar, [
    save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS $1"
    AC_LINK_IFELSE(
      [AC_LANG_PROGRAM([], [])],
      [AS_VAR_SET(cachevar, [yes])],
      [AS_VAR_SET(cachevar, [no])])
    LDFLAGS="$save_LDFLAGS"
  ])
  AS_IF([test x"AS_VAR_GET(cachevar)" = xyes], [$3], [$4])
  AS_VAR_POPDEF([cachevar])
])

dnl Check if the linker supports the given flags
dnl By default, add them to LDFLAGS if the linker does
AC_DEFUN([CHECK_LDFLAGS], [
  CHECK_LDFLAGS_BASE([$1], [whether the linker accepts $1],
    [m4_default([$2], [LDFLAGS="$1 $LDFLAGS"])],
    [m4_default([$3], [:])])
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

dnl Gtk -- note that this requires GtkGLExt
CHECK_FRONTEND([gtk], [Gtk+-2.0 / GtkGLExt], [
  PKG_CHECK_MODULES([GTK], [gtk+-2.0 gtkglext-1.0], [HAVE_GTK=yes], [:])
])

dnl From the changelog: "SDL 1.2.10 is a major release, featuring a
dnl revamp of the build system and many API improvements and bug fixes."
CHECK_FRONTEND([sdl], [LibSDL (experimental)], [
  AM_PATH_SDL([1.2.10], [HAVE_SDL=yes])
])

dnl Check that we have a front-end enabled
if test -z "$have_frontend" ; then
  AC_MSG_ERROR([no frontends found, try installing development packages])
fi

dnl Check for OpenGL
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
AC_SUBST([OPENGL_LIBS])
AC_SUBST([OPENGL_CFLAGS])

dnl Check for some way to load images
if test -z "$have_image" ; then
  PKG_CHECK_MODULES([LIBPNG], [libpng], [
    AC_DEFINE([HAVE_LIBPNG], [1], [LibPNG is available])
    HAVE_LIBPNG=yes
  ], [:])
  AC_CHECK_LIB([jpeg], [jpeg_start_decompress], [
    AC_DEFINE([HAVE_LIBJPEG], [1], [LibJPEG is available])
    LIBJPEG_CFLAGS=
    LIBJPEG_LIBS=-ljpeg
    HAVE_LIBJPEG=yes
  ])
  if test -n "$HAVE_LIBPNG" && test -n "$HAVE_LIBJPEG" ; then
    have_image='LibPNG, LibJPEG'
    IMAGE_LIBS="$LIBPNG_LIBS $LIBJPEG_LIBS"
  fi
fi
if test -z "$have_image" ; then
  AC_MSG_ERROR([cannot load images, try installing LibPNG and LibJPEG])
fi
AC_SUBST([HAVE_LIBPNG])
AC_SUBST([HAVE_LIBJPEG])
AC_SUBST([LIBPNG_CFLAGS])
AC_SUBST([LIBPNG_LIBS])
AC_SUBST([LIBJPEG_CFLAGS])
AC_SUBST([LIBJPEG_LIBS])

dnl Check for some way to render type
if test -z "$have_type" ; then
  PKG_CHECK_MODULES([PANGO], [pangocairo], [
    AC_DEFINE([HAVE_PANGOCAIRO], [1], [PangoCairo is available])
    have_type='Pango with Cairo backend'
    HAVE_PANGO=yes
    TYPE_LIBS="$PANGO_LIBS"
  ], [
    PKG_CHECK_MODULES([PANGOOTHER], [pango], [
      AC_MSG_WARN([Pango is installed but does not have the Cairo backend])
    ], [:])
  ])
fi
if test -z "$have_type" ; then
  AC_MSG_ERROR([cannot render type, try installing Pango])
fi
AC_SUBST([HAVE_PANGO])
AC_SUBST([PANGO_CFLAGS])
AC_SUBST([TYPE_LIBS])

dnl Check for audio
PKG_CHECK_MODULES([LIBASOUND], [alsa], [
  AC_DEFINE([HAVE_LIBASOUND], [1], [LibPNG is available])
  HAVE_LIBASOUND=yes
], [:])

dnl Enable warnings with --enable-warnings
AC_ARG_ENABLE(warnings,
  AS_HELP_STRING([--enable-warnings], [enable warnings for GCC]),
  [enable_warnings=$enableval], [enable_warnings=no])
if test "x$enable_warnings" != xno ; then
  CWARN="-Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wwrite-strings -Wmissing-prototypes"
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

dnl Check for linker options we want
dnl However, GNU binutils (at least) is very lax about command line
dnl parameters, so we want to only give it exactly the parameters it
dnl is supposed to support rather than let it guess what we mean.
dnl So we target options to the linkers we know about.
dnl The options we are checking for are not that important anyway.

dnl Guess the linker vendor
dnl Binutils lets us get away with -Wl,-v or -Wl,--version
dnl Mac OS X makes us run ld directly
ld_info=$($CC -Wl,--version 2>/dev/null) || \
  ld_info=$(ld -v 2>/dev/null) || \
  ld_info=unknown
case "$ld_info" in
  "GNU ld "*) ld_vendor=gnu ;;
  "Low Level Virtual Machine "*) ld_vendor=llvm ;;
  *)
    case "$UNAME_S" in
      Darwin) ld_vendor=apple ;;
      *) ld_vendor=unknown ;;
    esac
    ;;
esac

dnl Add the options we want, depending on linker vendor
case "$ld_vendor" in
  gnu)
    CHECK_LDFLAGS([-Wl,--gc-sections])
    CHECK_LDFLAGS([-Wl,--as-needed])
    ;;
  apple)
    CHECK_LDFLAGS([-Wl,-dead_strip])
    CHECK_LDFLAGS([-Wl,-dead_strip_dylibs])
    ;;
esac

dnl Remove -Wl,--export-dynamic from GTK_LIBS
dnl It's part of 'gmodule', but we don't need it
GTK_LIBS="$(echo " $GTK_LIBS " | sed 's/  *-Wl,--export-dynamic  */ /')"

AC_OUTPUT

echo
echo "Front ends:      $have_frontend"
echo "Image loading:   $have_image"
echo "Type rendering:  $have_type"
echo "Warnings:        $warnings"
