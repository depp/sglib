AUTOMAKE_OPTIONS = 1.5 foreign

TESTS = cvar strbuf
check_PROGRAMS = cvar strbuf

AM_CXXFLAGS = @CXXWARN@ -I@top_srcdir@/src
AM_CFLAGS = @CWARN@ -I@top_srcdir@/src

cvar_LDADD = ../src/libmimp.a
cvar_SOURCES = cvar.c

strbuf_LDADD = ../src/libmimp.a
strbuf_SOURCES = strbuf.c
