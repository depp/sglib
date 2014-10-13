all: test_xform test_write
clean:
	rm -f test_xform test_write *.o

include ../common.mak
VPATH = ../../src/core

test_xform: sprite_xform.o test_xform.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

test_write: sprite_write.o test_write.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
