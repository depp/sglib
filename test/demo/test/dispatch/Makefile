targets = common

all: $(targets)

VPATH = ../../sglib/src/base
CFLAGS = -O2 -g
CWARN = -Wall -Wextra -Wpointer-arith -Wno-sign-compare -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Werror

clean:
	rm -f $(targets)

common: common.c dispatch_common.c
	$(CC) -I../../sglib/src/base $(CFLAGS) $(CWARN) -o $@ $^
