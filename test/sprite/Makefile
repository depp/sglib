all: test_xform
clean:
	rm -f test_xform *.o

include ../common.mak
VPATH = ../../src/core

test_xform: sprite_xform.o test_xform.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
