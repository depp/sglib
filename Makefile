all:

SHELL = /bin/sh
RM = rm -f
srcdir = $(CURDIR)

all: run.sh
	$(MAKE) -C src all

clean:
	$(MAKE) -C src clean
	$(RM) run.sh

run.sh: scripts/run.sh.in
	sed \
		-e 's,@SRCDIR@,$(srcdir),g' \
		-e 's,@BUILDDIR@,$(srcdir),g' \
		-e 's,@SHELL@,$(SHELL),g' \
		$< > $@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@

.PHONY: all clean
