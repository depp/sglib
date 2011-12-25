AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects
SUBDIRS = . src tests

EXTRA_DIST = src/run.sh.in
CLEANFILES = run.sh
noinst_SCRIPTS = run.sh

run.sh : $(srcdir)/src/run.sh.in Makefile
	sed \
		-e 's,@''abs_top_srcdir@,@abs_top_srcdir@,g' \
		-e 's,@''abs_top_builddir@,@abs_top_builddir@,g' \
		-e 's,@''shell@,@shell@,g' \
		$(srcdir)/src/run.sh.in > $@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@
