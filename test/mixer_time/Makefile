all: mixer_time
clean:
	rm -f mixer_time *.o

include ../common.mak
LIBS += -lm
VPATH = ../../src/mixer

mixer_time: mixer_time.o time.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean
