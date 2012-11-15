AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . tests

AM_CPPFLAGS = -I@srcdir@/include

lib_LTLIBRARIES = libpce.la

pkginclude_HEADERS = \
    include/libpce/arch.h \
    include/libpce/attribute.h \
    include/libpce/byteorder.h \
    include/libpce/byteswap.h \
    include/libpce/cpu.h \
    include/libpce/strbuf.h

libpce_la_SOURCES = \
    lib/cpu.c \
    lib/strbuf.c
