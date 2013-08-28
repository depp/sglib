AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . tests

AM_CPPFLAGS = -I@srcdir@/include
AM_CFLAGS = @CWARN@ -pthread

lib_LTLIBRARIES = libpce.la

pkginclude_HEADERS = \
	include/libpce/arch.h \
	include/libpce/atomic.h \
	include/libpce/attribute.h \
	include/libpce/binary.h \
	include/libpce/byteorder.h \
	include/libpce/cpu.h \
	include/libpce/hash.h \
	include/libpce/strbuf.h \
	include/libpce/thread.h \
	include/libpce/util.h

libpce_la_LDFLAGS = -pthread

libpce_la_SOURCES = \
	lib/cpu.c \
	lib/hash.c \
	lib/hashtable.c \
	lib/strbuf.c \
	lib/thread_pthread.c

EXTRA_DIST = \
	lib/thread_windows.c
