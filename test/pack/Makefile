all: pack
clean:
	rm -f pack *.o

include ../common.mak
VPATH = ../../src/core

pack: pack.o test.o error.o logtest.o rand.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
