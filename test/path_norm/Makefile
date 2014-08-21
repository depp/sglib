all: path_norm
clean:
	rm -f path_norm *.o

include ../common.mak
VPATH = ../../src/core

path_norm: path_norm.o test.o error.o logtest.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
